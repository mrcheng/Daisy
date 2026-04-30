#include "daisy_seed.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
Oscillator osc;
Svf        lowpass;

constexpr float kBaseFreqHz       = 56.0f;
constexpr float kPitchAmountHz    = 112.0f;
constexpr float kPitchDecaySec    = 0.055f;
constexpr float kAmpDecaySec      = 0.360f;
constexpr float kLowpassCutoffHz  = 760.0f;
constexpr float kResonance        = 0.62f;
constexpr float kOutputGain       = 0.70f;
constexpr float kRepeatTimeSec    = 0.750f;

float    amp_env;
float    pitch_env;
float    amp_decay;
float    pitch_decay;
uint32_t repeat_samples;
uint32_t repeat_countdown;

static void TriggerKick()
{
    amp_env          = 1.0f;
    pitch_env        = 1.0f;
    repeat_countdown = repeat_samples;
    osc.Reset();
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(repeat_countdown == 0)
        {
            TriggerKick();
        }

        repeat_countdown--;

        float freq = kBaseFreqHz + (kPitchAmountHz * pitch_env);
        osc.SetFreq(freq);

        float sig = osc.Process() * amp_env;
        lowpass.Process(sig);
        sig = lowpass.Low() * kOutputGain;

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

    amp_decay       = expf(-1.0f / (kAmpDecaySec * sample_rate));
    pitch_decay     = expf(-1.0f / (kPitchDecaySec * sample_rate));
    repeat_samples  = static_cast<uint32_t>(kRepeatTimeSec * sample_rate);
    repeat_countdown = 0;

    hw.StartAudio(AudioCallback);

    while(1) {}
}
