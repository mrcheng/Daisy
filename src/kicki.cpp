#include "daisy_seed.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
Oscillator osc;
Svf        lowpass;
Overdrive  overdrive;
Wavefolder wavefolder;

constexpr float kBaseFreqHz       = 56.0f;
constexpr float kPitchAmountHz    = 112.0f;
constexpr float kPitchDecaySec    = 0.055f;
constexpr float kAttackSec        = 0.004f;
constexpr float kAmpDecaySec      = 0.360f;
constexpr float kSustainLevel     = 0.04f;
constexpr float kReleaseSec       = 0.060f;
constexpr float kLowpassCutoffHz  = 760.0f;
constexpr float kResonance        = 0.62f;
constexpr bool  kSimpleClipOn     = false;
constexpr float kSimpleClipDrive  = 0.25f;
constexpr bool  kOverdriveOn      = false;
constexpr float kOverdriveDrive   = 0.35f;
constexpr bool  kWavefolderOn     = false;
constexpr float kWavefolderDrive  = 0.20f;
constexpr bool  kSaturationOn     = false;
constexpr bool  kSaturationPost   = false;
constexpr float kSaturationDrive  = 0.35f;
constexpr float kOutputGain       = 0.70f;

float pitch_env;
float pitch_decay;
uint32_t env_sample;
uint32_t attack_samples;
uint32_t decay_samples;
uint32_t release_samples;

static void TriggerKick()
{
    pitch_env = 1.0f;
    env_sample = 0;
    osc.Reset();
}

static float AmpEnvelope()
{
    if(env_sample < attack_samples)
    {
        return static_cast<float>(env_sample) / static_cast<float>(attack_samples);
    }

    uint32_t decay_end = attack_samples + decay_samples;
    if(env_sample < decay_end)
    {
        float t = static_cast<float>(env_sample - attack_samples)
                  / static_cast<float>(decay_samples);
        return 1.0f + (kSustainLevel - 1.0f) * t;
    }

    uint32_t release_end = decay_end + release_samples;
    if(env_sample < release_end)
    {
        float t = static_cast<float>(env_sample - decay_end)
                  / static_cast<float>(release_samples);
        return kSustainLevel * (1.0f - t);
    }

    return 0.0f;
}

static float Clamp(float x, float lo, float hi)
{
    return x < lo ? lo : x > hi ? hi : x;
}

static float HardClip(float sig, float drive)
{
    float driven = sig * (1.0f + drive * 16.0f);
    return Clamp(driven, -1.0f, 1.0f);
}

static float Saturate(float sig, float drive)
{
    float driven = sig * (1.0f + drive * 20.0f);
    return driven / (1.0f + fabsf(driven));
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float freq = kBaseFreqHz + (kPitchAmountHz * pitch_env);
        osc.SetFreq(freq);

        float sig = osc.Process() * AmpEnvelope();

        if(kSimpleClipOn)
            sig = HardClip(sig, kSimpleClipDrive);
        if(kOverdriveOn)
            sig = overdrive.Process(sig);
        if(kWavefolderOn)
            sig = wavefolder.Process(sig);
        if(kSaturationOn && !kSaturationPost)
            sig = Saturate(sig, kSaturationDrive);

        lowpass.Process(sig);
        sig = lowpass.Low();

        if(kSaturationOn && kSaturationPost)
            sig = Saturate(sig, kSaturationDrive);

        sig *= kOutputGain;

        pitch_env *= pitch_decay;
        env_sample++;

        out[0][i] = sig;
        out[1][i] = sig;
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);

    float sample_rate = hw.AudioSampleRate();

    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetAmp(1.0f);

    lowpass.Init(sample_rate);
    lowpass.SetFreq(kLowpassCutoffHz);
    lowpass.SetRes(kResonance);
    lowpass.SetDrive(0.3f);

    overdrive.Init();
    overdrive.SetDrive(kOverdriveDrive);

    wavefolder.Init();
    wavefolder.SetGain(1.0f + kWavefolderDrive * 8.0f);
    wavefolder.SetOffset(0.0f);

    pitch_decay    = expf(-1.0f / (kPitchDecaySec * sample_rate));
    attack_samples = static_cast<uint32_t>(kAttackSec * sample_rate);
    decay_samples  = static_cast<uint32_t>(kAmpDecaySec * sample_rate);
    release_samples = static_cast<uint32_t>(kReleaseSec * sample_rate);

    if(attack_samples == 0)
        attack_samples = 1;
    if(decay_samples == 0)
        decay_samples = 1;
    if(release_samples == 0)
        release_samples = 1;

    TriggerKick();

    hw.StartAudio(AudioCallback);

    while(1) {}
}
