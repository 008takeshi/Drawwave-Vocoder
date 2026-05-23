#include "DrawableLfo.h"
#include <cmath>

DrawableLfo::DrawableLfo()
{
    // Default table: sine
    for (int i = 0; i < TableSize; ++i)
    {
        const float v = std::sin(juce::MathConstants<float>::twoPi
                                 * (float)i / (float)TableSize);
        tables[0][i] = v;
        tables[1][i] = v;
    }
}

void DrawableLfo::prepare(double sr) noexcept
{
    sampleRate = sr;
    phase      = 0.0f;
    atomicPhase.store(0.0f, std::memory_order_relaxed);
}

void DrawableLfo::setTable(const std::vector<float>& samples)
{
    const int writeIdx = 1 - activeIdx.load(std::memory_order_acquire);
    const int n        = std::min((int)samples.size(), TableSize);
    for (int i = 0; i < n; ++i)
        tables[writeIdx][i] = samples[(size_t)i];
    for (int i = n; i < TableSize; ++i)
        tables[writeIdx][i] = 0.0f;
    activeIdx.store(writeIdx, std::memory_order_release);
}

void DrawableLfo::prepareBlock(float rateHz, bool syncEnabled, float syncBeats,
                                double bpm, int numSamples) noexcept
{
    if (syncEnabled && bpm > 0.0 && syncBeats > 1e-6f)
        rateHz = (float)(bpm / 60.0) / syncBeats;

    rateHz = juce::jlimit(0.01f, 200.0f, rateHz);

    const float increment = rateHz * (float)TableSize
                            / (float)sampleRate * (float)numSamples;

    // Sample active table at current phase
    const float* tbl = tables[activeIdx.load(std::memory_order_acquire)];
    const int    idx  = (int)phase & (TableSize - 1);
    const float  frac = phase - (float)idx;
    const float  s0   = tbl[(size_t)idx];
    const float  s1   = tbl[(size_t)((idx + 1) & (TableSize - 1))];
    currentOutput     = s0 + frac * (s1 - s0);

    // Advance and wrap (use fmod to handle increment > TableSize at extreme rates)
    phase += increment;
    if (phase >= (float)TableSize)
        phase = std::fmod(phase, (float)TableSize);

    atomicPhase.store(phase / (float)TableSize, std::memory_order_relaxed);
}
