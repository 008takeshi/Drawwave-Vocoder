#pragma once
#include "MipmapTable.h"

class WavetableOscillator
{
public:
    void setTable     (const MipmapTable* t) noexcept { table = t; layerPtr = nullptr; }
    void setFrequency (float freqHz, double sr) noexcept;
    void reset        () noexcept { phase = 0.0f; }

    // Call once per block before processSample() to cache the mipmap layer.
    void prepareBlock() noexcept;
    float processSample() noexcept;

    bool isReady() const noexcept { return table != nullptr && table->isReady(); }

private:
    const MipmapTable* table     = nullptr;
    const float*       layerPtr  = nullptr;
    float phase                  = 0.0f;
    float phaseIncrement         = 0.0f;
    float currentFreq            = 0.0f;
    double currentSampleRate     = 44100.0;
};
