#include "VocoderCore.h"
#include <cmath>

VocoderCore::VocoderCore()
{
    resetBandGains();
}

void VocoderCore::prepare(double sr, int blockSize,
                           int bandCount, float fLow, float fHigh,
                           float qVal, float atkMs, float relMs)
{
    sampleRate      = sr;
    samplesPerBlock = blockSize;
    numBands.store(juce::jlimit(1, MaxBands, bandCount), std::memory_order_relaxed);
    freqLow   = fLow;
    freqHigh  = fHigh;
    q         = qVal;
    attackMs  = atkMs;
    releaseMs = relMs;

    for (auto& a : bandLevels) a.store(0.0f, std::memory_order_relaxed);

    rebuildBanks();
}

void VocoderCore::rebuildBanks()
{
    const int nb = numBands.load(std::memory_order_relaxed);

    analyzeBank.prepare(sampleRate, nb, freqLow, freqHigh, q);
    synthBank.prepare  (sampleRate, nb, freqLow, freqHigh, q);

    for (int i = 0; i < nb; ++i)
    {
        auto idx = (size_t)i;
        envFollowers[idx].prepare(sampleRate);
        envFollowers[idx].setAttackMs(attackMs);
        envFollowers[idx].setReleaseMs(releaseMs);
    }

    // Pre-allocate scratch buffers — no heap alloc in processBlock
    analyzeBands.resize((size_t)nb);
    synthBands.resize  ((size_t)nb);
    for (int b = 0; b < nb; ++b)
    {
        analyzeBands[(size_t)b].assign((size_t)samplesPerBlock, 0.0f);
        synthBands  [(size_t)b].assign((size_t)samplesPerBlock, 0.0f);
    }
}

void VocoderCore::applyLfoFormantShift(float totalSemitones) noexcept
{
    synthBank.applyFormantShiftRT(totalSemitones);
}

void VocoderCore::process(const float* carrier, const float* modulator,
                           float* output, int numSamples)
{
    analyzeBank.processBlock(modulator, numSamples, analyzeBands);
    synthBank.processBlock  (carrier,   numSamples, synthBands);

    std::fill(output, output + numSamples, 0.0f);

    const int nb = numBands.load(std::memory_order_relaxed);

    float gainSnapshot[MaxBands];
    for (int b = 0; b < nb; ++b)
        gainSnapshot[b] = bandGains[(size_t)b].load(std::memory_order_relaxed);

    // Apply band gain tilt: bandGain multiplied by a linear slope across bands.
    const float tilt = tiltOffset;
    if (std::abs(tilt) > 1e-6f && nb > 1)
    {
        for (int b = 0; b < nb; ++b)
        {
            const float norm = (float)b / (float)(nb - 1) - 0.5f;  // [-0.5, 0.5]
            gainSnapshot[b] = juce::jlimit(0.0f, 2.0f,
                                            gainSnapshot[b] * (1.0f + tilt * norm));
        }
    }

    for (int b = 0; b < nb; ++b)
    {
        const auto bidx = (size_t)b;
        float peakEnv = 0.0f;
        for (int s = 0; s < numSamples; ++s)
        {
            float env = envFollowers[bidx].process(analyzeBands[bidx][(size_t)s]);
            peakEnv   = std::max(peakEnv, env);
            output[s] += synthBands[bidx][(size_t)s] * env * gainSnapshot[b];
        }
        bandLevels[bidx].store(peakEnv, std::memory_order_relaxed);
    }

    // Soft-clip and HP/LP are applied in OutputStage (PluginProcessor::processBlock)
}

void VocoderCore::setBandCount(int n)
{
    numBands.store(juce::jlimit(1, MaxBands, n), std::memory_order_relaxed);
    rebuildBanks();
}

void VocoderCore::setQ(float qVal)
{
    if (std::memcmp(&q, &qVal, sizeof(float)) == 0) return;
    q = qVal;
    rebuildBanks();
}

void VocoderCore::setAttackMs(float ms)
{
    attackMs = ms;
    const int nb = numBands.load(std::memory_order_relaxed);
    for (int i = 0; i < nb; ++i)
        envFollowers[(size_t)i].setAttackMs(ms);
}

void VocoderCore::setReleaseMs(float ms)
{
    releaseMs = ms;
    const int nb = numBands.load(std::memory_order_relaxed);
    for (int i = 0; i < nb; ++i)
        envFollowers[(size_t)i].setReleaseMs(ms);
}

float VocoderCore::getModulatorEnv() const noexcept
{
    const int nb = numBands.load(std::memory_order_relaxed);
    if (nb <= 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < nb; ++i)
        sum += bandLevels[(size_t)i].load(std::memory_order_relaxed);
    return sum / (float)nb;
}

float VocoderCore::getBandLevel(int band) const noexcept
{
    const int nb = numBands.load(std::memory_order_relaxed);
    if (band < 0 || band >= nb) return 0.0f;
    return bandLevels[(size_t)band].load(std::memory_order_relaxed);
}

void VocoderCore::setBandGain(int band, float gain)
{
    if (band >= 0 && band < MaxBands)
        bandGains[(size_t)band].store(juce::jlimit(0.0f, 2.0f, gain),
                                      std::memory_order_relaxed);
}

float VocoderCore::getBandGain(int band) const noexcept
{
    if (band < 0 || band >= MaxBands) return 1.0f;
    return bandGains[(size_t)band].load(std::memory_order_relaxed);
}

void VocoderCore::resetBandGains()
{
    for (auto& g : bandGains) g.store(1.0f, std::memory_order_relaxed);
}
