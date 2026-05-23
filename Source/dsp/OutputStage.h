#pragma once
#include <juce_dsp/juce_dsp.h>

// Post-vocoder output stage: HP filter → LP filter → tanh soft-clip.
// All processing runs on the audio thread; parameters are passed per-block
// by the caller (read from APVTS raw pointers in processBlock).
class OutputStage
{
public:
    OutputStage() = default;
    void prepare(double sampleRate, int samplesPerBlock);

    // Audio thread: apply HP/LP filtering then soft-clip to signal in-place.
    void process(float* signal, int numSamples, float hpHz, float lpHz) noexcept;

private:
    juce::dsp::StateVariableTPTFilter<float> hpFilter;
    juce::dsp::StateVariableTPTFilter<float> lpFilter;
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputStage)
};
