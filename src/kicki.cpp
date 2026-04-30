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
constexpr float kAmpDecaySec      = 0.360f;
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

float amp_env;
float pitch_env;
float amp_decay;
float pitch_decay;

static void TriggerKick()
{
    amp_env   = 1.0f;
    pitch_env = 1.0f;
    osc.Reset();
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

        float sig = osc.Process() * amp_env;

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

        amp_env *= amp_decay;
        pitch_env *= pitch_decay;

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

    amp_decay       = expf(-1.0f / (kAmpDecaySec * sample_rate));
    pitch_decay     = expf(-1.0f / (kPitchDecaySec * sample_rate));

    TriggerKick();

    hw.StartAudio(AudioCallback);

    while(1) {}
}
