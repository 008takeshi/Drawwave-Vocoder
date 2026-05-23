#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <atomic>

// 512-sample LFO with double-buffered table swap.
// Audio-thread methods: prepareBlock(), getOutput(), retrigger().
// Message-thread method: setTable().
class DrawableLfo
{
public:
    static constexpr int TableSize = 512;

    DrawableLfo();

    void prepare(double sampleRate) noexcept;

    // Message thread: copy samples into the inactive table, then swap atomically.
    void setTable(const std::vector<float>& samples);

    // Audio thread: advance LFO by one block and compute output.
    // All parameters are passed by the caller (read from APVTS on audio thread).
    // syncBeats: beat-length of one LFO cycle (e.g. 0.25 = 1/4 note).
    void prepareBlock(float rateHz, bool syncEnabled, float syncBeats,
                      double bpm, int numSamples) noexcept;

    float getOutput()          const noexcept { return currentOutput; }

    // Approximate phase [0, 1] for GUI playhead — relaxed read, one block stale.
    float getPhaseNormalized() const noexcept
        { return atomicPhase.load(std::memory_order_relaxed); }

    // Audio thread only: reset phase to 0 (call on noteOn if retrigger is enabled).
    void retrigger() noexcept { phase = 0.0f; }

private:
    float tables[2][TableSize] {};
    std::atomic<int> activeIdx { 0 };

    float  phase         = 0.0f;
    float  currentOutput = 0.0f;
    double sampleRate    = 44100.0;

    std::atomic<float> atomicPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrawableLfo)
};
