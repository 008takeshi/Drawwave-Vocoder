#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Simple voiced/unvoiced detector using ZCR and RMS.
// Returns true when the signal appears voiced (speech present).
class UnvoicedDetector
{
public:
    UnvoicedDetector() = default;
    void prepare(double sampleRate) noexcept;
    bool process(const float* signal, int numSamples) noexcept;
    void setSensitivity(float s) noexcept
        { sensitivity = juce::jlimit(0.0f, 1.0f, s); }

private:
    float sensitivity = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnvoicedDetector)
};
