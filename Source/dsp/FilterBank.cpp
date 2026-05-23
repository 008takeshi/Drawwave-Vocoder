#include "FilterBank.h"
#include <cmath>

// ---------------------------------------------------------------------------
// Biquad
// ---------------------------------------------------------------------------

void FilterBank::Biquad::setBandPass(double sr, float freq, float qVal) noexcept
{
    const float w0    = juce::MathConstants<float>::twoPi * freq / (float)sr;
    const float sinW0 = std::sin(w0);
    const float cosW0 = std::cos(w0);
    const float alpha = sinW0 / (2.0f * qVal);
    const float a0inv = 1.0f / (1.0f + alpha);

    b0 =  alpha * a0inv;
    b2 = -alpha * a0inv;
    a1 = -2.0f * cosW0 * a0inv;
    a2 = (1.0f - alpha) * a0inv;
}

// ---------------------------------------------------------------------------
// FilterBank
// ---------------------------------------------------------------------------

void FilterBank::prepare(double sr, int bandCount,
                          float fLow, float fHigh, float qVal)
{
    sampleRate = sr;
    numBands   = juce::jlimit(1, MaxBands, bandCount);
    freqLow    = fLow;
    freqHigh   = fHigh;
    q          = qVal;

    filters.resize((size_t)numBands);
    for (auto& f : filters) f.reset();

    rebuildCoeffs();
}

void FilterBank::rebuildCoeffs()
{
    const float logLow  = std::log2(freqLow);
    const float logHigh = std::log2(freqHigh);
    const float step    = (numBands > 1)
                          ? (logHigh - logLow) / (float)(numBands - 1)
                          : 0.0f;

    for (int i = 0; i < numBands; ++i)
    {
        const float cf = juce::jlimit(20.0f, (float)(sampleRate * 0.45),
                                       std::pow(2.0f, logLow + step * (float)i));
        baseCenterFreqs[i] = cf;
        filters[(size_t)i].setBandPass(sampleRate, cf, this->q);
    }
}

void FilterBank::applyFormantShiftRT(float semitones) noexcept
{
    const float factor = std::pow(2.0f, semitones / 12.0f);
    for (int i = 0; i < numBands; ++i)
    {
        const float cf = juce::jlimit(20.0f, (float)(sampleRate * 0.45),
                                       baseCenterFreqs[i] * factor);
        filters[(size_t)i].setBandPass(sampleRate, cf, this->q);
    }
}

void FilterBank::processBlock(const float* input, int numSamples,
                               std::vector<std::vector<float>>& bands)
{
    // bands is pre-allocated by VocoderCore::rebuildBanks(); no resize here
    for (int b = 0; b < numBands; ++b)
        for (int s = 0; s < numSamples; ++s)
            bands[(size_t)b][(size_t)s] = filters[(size_t)b].process(input[s]);
}
