#include "OutputStage.h"
#include <cmath>

void OutputStage::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec{ sr, (juce::uint32)samplesPerBlock, 1 };

    hpFilter.prepare(spec);
    hpFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    hpFilter.setCutoffFrequency(20.0f);
    hpFilter.setResonance(0.707f);

    lpFilter.prepare(spec);
    lpFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lpFilter.setCutoffFrequency(20000.0f);
    lpFilter.setResonance(0.707f);
}

void OutputStage::process(float* signal, int numSamples,
                           float hpHz, float lpHz) noexcept
{
    // Update filter cutoffs (cheap coefficient calc, audio-thread safe)
    hpFilter.setCutoffFrequency(juce::jlimit(20.0f,   (float)(sampleRate * 0.49), hpHz));
    lpFilter.setCutoffFrequency(juce::jlimit(100.0f,  (float)(sampleRate * 0.49), lpHz));

    // HP → LP → tanh
    for (int s = 0; s < numSamples; ++s)
    {
        float x = hpFilter.processSample(0, signal[s]);
        x       = lpFilter.processSample(0, x);
        signal[s] = std::tanh(x);
    }
}
