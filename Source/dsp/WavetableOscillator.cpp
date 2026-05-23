#include "WavetableOscillator.h"

void WavetableOscillator::setFrequency(float freqHz, double sr) noexcept
{
    currentFreq       = freqHz;
    currentSampleRate = sr;
    phaseIncrement    = freqHz * (float)MipmapTable::TableSize / (float)sr;
}

void WavetableOscillator::prepareBlock() noexcept
{
    if (table != nullptr)
        layerPtr = table->getLayerForIncrement(phaseIncrement);
}

float WavetableOscillator::processSample() noexcept
{
    if (layerPtr == nullptr) return 0.0f;

    // Linear interpolation between adjacent table samples
    const int   idx  = (int)phase & (MipmapTable::TableSize - 1);
    const float frac = phase - (float)(int)phase;
    const float s0   = layerPtr[(size_t)idx];
    const float s1   = layerPtr[(size_t)((idx + 1) & (MipmapTable::TableSize - 1))];
    const float out  = s0 + frac * (s1 - s0);

    phase += phaseIncrement;
    if (phase >= (float)MipmapTable::TableSize)
        phase -= (float)MipmapTable::TableSize;

    return out;
}
