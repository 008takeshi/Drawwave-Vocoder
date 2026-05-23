#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// BPF bank: two cascaded 2nd-order sections per band (4th-order steeper roll-off,
// narrower bandwidth than a single stage — not a Butterworth design).
// All coefficient writes happen on the audio thread via applyFormantShiftRT,
// which reads baseCenterFreqs set during prepare().
class FilterBank
{
public:
    static constexpr int MaxBands = 32;

    void prepare(double sampleRate, int bandCount,
                 float freqLow, float freqHigh, float q);

    // Audio thread: filter input into per-band output buffers (pre-allocated by caller).
    void processBlock(const float* input, int numSamples,
                      std::vector<std::vector<float>>& bands);

    int getBandCount() const noexcept { return numBands; }

    // Audio thread: recompute coefficients with a formant shift applied to baseCenterFreqs.
    // Safe to call every block; does not heap-allocate.
    void applyFormantShiftRT(float semitones) noexcept;

private:
    // Direct Form II Transposed 2nd-order section.
    // b1 = 0 for BPF, so the update equations simplify slightly.
    struct Biquad
    {
        float b0 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float w1 = 0.0f, w2 = 0.0f;  // delay-line state

        void setBandPass(double sr, float freq, float qVal) noexcept;
        void reset() noexcept { w1 = w2 = 0.0f; }

        float process(float x) noexcept
        {
            float y = b0 * x + w1;
            w1 = -a1 * y + w2;
            w2 = b2 * x - a2 * y;
            return y;
        }
    };

    // Two cascaded biquads per band → 4th-order BPF
    struct BandFilter
    {
        Biquad s1, s2;
        void setBandPass(double sr, float freq, float qVal) noexcept
            { s1.setBandPass(sr, freq, qVal); s2.setBandPass(sr, freq, qVal); }
        void reset() noexcept { s1.reset(); s2.reset(); }
        float process(float x) noexcept { return s2.process(s1.process(x)); }
    };

    void rebuildCoeffs();

    double sampleRate = 44100.0;
    int    numBands   = 16;
    float  freqLow    = 80.0f;
    float  freqHigh   = 11000.0f;
    float  q          = 4.0f;

    // Unshifted log-spaced centre frequencies — written only during prepare().
    float baseCenterFreqs[MaxBands] {};

    std::vector<BandFilter> filters;
};
