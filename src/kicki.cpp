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
AnalogControl master_dist_knob;

constexpr float kBaseFreqHz       = 40.0f;
constexpr float kPitchAmountHz    = 186.0f;
constexpr float kPitchDecaySec    = 0.045f;
constexpr float kAttackSec        = 0.007f;
constexpr float kAmpDecaySec      = 0.070f;
constexpr float kSustainLevel     = 0.75f;
constexpr float kReleaseSec       = 0.120f;
constexpr float kLowpassCutoffHz  = 790.0f;
constexpr float kResonance        = 0.78f;
constexpr bool  kSimpleClipOn     = true;
constexpr float kSimpleClipDrive  = 0.02f;
constexpr bool  kOverdriveOn      = true;
constexpr float kOverdriveDrive   = 0.01f;
constexpr bool  kWavefolderOn     = true;
constexpr float kWavefolderDrive  = 0.04f;
constexpr bool  kSaturationOn     = true;
constexpr bool  kSaturationPost   = true;
constexpr float kSaturationDrive  = 0.07f;
constexpr bool  kTwisterOn        = true;
constexpr float kTwisterPull      = 0.35f;
constexpr float kTwisterRecoverySec = 0.220f;
constexpr float kTwisterStruggle  = 0.32f;
constexpr float kOutputGain       = 0.70f;

// Reserved for a future 10k pot: 3V3 -> pot -> GND, wiper to Daisy Seed A0/D15.
constexpr bool  kMasterDistKnobEnabled = false;
constexpr Pin   kMasterDistKnobPin     = seed::A0;
constexpr float kMasterDistMin         = 0.00f;
constexpr float kMasterDistMax         = 2.00f;

float pitch_env;
float pitch_decay;
float twister_env;
float twister_sag;
float twister_attack_coeff;
float twister_release_coeff;
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

static float KickiTwister(float sig)
{
    if(!kTwisterOn)
        return sig;

    float level = fabsf(sig);
    float coeff = level > twister_env ? twister_attack_coeff : twister_release_coeff;
    twister_env = coeff * twister_env + (1.0f - coeff) * level;

    float over       = Clamp((twister_env - 0.18f) / 0.82f, 0.0f, 1.0f);
    float target_sag = over * kTwisterPull * 0.55f;
    float sag_rate   = target_sag > twister_sag ? 0.18f : (1.0f - twister_release_coeff);
    twister_sag += (target_sag - twister_sag) * sag_rate;

    float strain = kTwisterStruggle * twister_sag;
    float dirty  = sig + sinf(sig * 24.0f) * strain * 0.025f;
    dirty        = Saturate(dirty, strain * 0.35f);

    return dirty * (1.0f - twister_sag);
}

static float MasterDistMultiplier()
{
    if(!kMasterDistKnobEnabled)
        return 1.0f;

    float knob = master_dist_knob.Process();
    return kMasterDistMin + knob * (kMasterDistMax - kMasterDistMin);
}

static void InitMasterDistKnob(float control_rate)
{
    if(!kMasterDistKnobEnabled)
        return;

    AdcChannelConfig adc_cfg[1];
    adc_cfg[0].InitSingle(kMasterDistKnobPin);
    hw.adc.Init(adc_cfg, 1);
    master_dist_knob.Init(hw.adc.GetPtr(0), control_rate);
    hw.adc.Start();
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    float master_dist      = MasterDistMultiplier();
    float simple_clip_drive = Clamp(kSimpleClipDrive * master_dist, 0.0f, 1.0f);
    float overdrive_drive   = Clamp(kOverdriveDrive * master_dist, 0.0f, 1.0f);
    float wavefolder_drive  = Clamp(kWavefolderDrive * master_dist, 0.0f, 1.0f);
    float saturation_drive  = Clamp(kSaturationDrive * master_dist, 0.0f, 1.0f);

    if(kOverdriveOn)
        overdrive.SetDrive(overdrive_drive);
    if(kWavefolderOn)
        wavefolder.SetGain(1.0f + wavefolder_drive * 8.0f);

    for(size_t i = 0; i < size; i++)
    {
        float freq = kBaseFreqHz + (kPitchAmountHz * pitch_env);
        osc.SetFreq(freq);

        float sig = osc.Process() * AmpEnvelope();

        if(kSimpleClipOn)
            sig = HardClip(sig, simple_clip_drive);
        if(kOverdriveOn)
            sig = overdrive.Process(sig);
        if(kWavefolderOn)
            sig = wavefolder.Process(sig);
        if(kSaturationOn && !kSaturationPost)
            sig = Saturate(sig, saturation_drive);

        lowpass.Process(sig);
        sig = lowpass.Low();

        if(kSaturationOn && kSaturationPost)
            sig = Saturate(sig, saturation_drive);

        sig = KickiTwister(sig);
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
    InitMasterDistKnob(hw.AudioCallbackRate());

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
    twister_env    = 0.0f;
    twister_sag    = 0.0f;
    twister_attack_coeff = expf(-1.0f / (0.003f * sample_rate));
    twister_release_coeff = expf(-1.0f / (kTwisterRecoverySec * sample_rate));
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
