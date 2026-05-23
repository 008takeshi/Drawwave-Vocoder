#include "MipmapTable.h"
#include <cmath>

MipmapTable::MipmapTable()
{
    std::vector<float> saw(TableSize);
    fillSaw(saw);
    buildInto(tables[0], saw);
    buildInto(tables[1], saw);
    activeIdx.store(0, std::memory_order_release);
}

MipmapTable::~MipmapTable()
{
    // Signal any running build to abort, then wait for it to finish.
    // This is safe to call even if buildThread is not joinable.
    if (buildThread.joinable())
        buildThread.join();
}

void MipmapTable::setTable(const std::vector<float>& samples)
{
    // Join the previous build before starting a new one so the thread
    // never outlives this object.
    if (buildThread.joinable())
        buildThread.join();

    std::vector<float> copy(samples);
    copy.resize(TableSize, 0.0f);

    buildThread = std::thread([this, c = std::move(copy)]() mutable {
        std::lock_guard<std::mutex> lock(buildMutex);
        const int inactive = 1 - activeIdx.load(std::memory_order_relaxed);
        buildInto(tables[(size_t)inactive], c);
        activeIdx.store(inactive, std::memory_order_release);
    });
}

const float* MipmapTable::getLayerForFreq(float freqHz, double sampleRate) const noexcept
{
    const float increment = freqHz * (float)TableSize / (float)sampleRate;
    return getLayerForIncrement(increment);
}

const float* MipmapTable::getLayerForIncrement(float increment) const noexcept
{
    const int idx = activeIdx.load(std::memory_order_acquire);
    if (idx < 0) return tables[0].layers[0].data();

    int layer = 0;
    if (increment > 1.0f)
    {
        float inc = increment;
        while (inc > 1.0f && layer < NumLayers - 1)
        {
            inc *= 0.5f;
            ++layer;
        }
    }

    return tables[(size_t)idx].layers[(size_t)layer].data();
}

// ---------------------------------------------------------------------------

void MipmapTable::fillSaw(std::vector<float>& buf)
{
    buf.resize(TableSize);
    for (int i = 0; i < TableSize; ++i)
        buf[(size_t)i] = 2.0f * (float)i / (float)TableSize - 1.0f;
}

void MipmapTable::buildInto(TableSet& dest, const std::vector<float>& src)
{
    juce::dsp::FFT fft(11);

    std::vector<float> fftBuf(TableSize * 2, 0.0f);
    const int n = (int)std::min(src.size(), (size_t)TableSize);
    for (int i = 0; i < n; ++i)
        fftBuf[(size_t)i] = src[(size_t)i];

    fft.performRealOnlyForwardTransform(fftBuf.data());
    const std::vector<float> spectrum = fftBuf;

    for (int layer = 0; layer < NumLayers; ++layer)
    {
        const int maxHarmonic = TableSize >> (layer + 1);

        fftBuf = spectrum;

        // Zero DC
        fftBuf[0] = 0.0f;
        fftBuf[1] = 0.0f;

        // Zero positive-frequency bins above maxHarmonic.
        // performRealOnlyInverseTransform fills the negative half automatically
        // from conjugates of the positive half, so we only touch bins 1..TableSize/2.
        for (int k = maxHarmonic + 1; k <= TableSize / 2; ++k)
        {
            fftBuf[(size_t)(k * 2)]     = 0.0f;
            fftBuf[(size_t)(k * 2 + 1)] = 0.0f;
        }

        fft.performRealOnlyInverseTransform(fftBuf.data());

        for (int i = 0; i < TableSize; ++i)
            dest.layers[(size_t)layer][(size_t)i] = fftBuf[(size_t)i];
    }
}
