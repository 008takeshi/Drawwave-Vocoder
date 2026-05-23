#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

class MipmapTable
{
public:
    static constexpr int TableSize = 2048;
    static constexpr int NumLayers = 11;

    using Layer = std::array<float, TableSize>;

    struct TableSet {
        std::array<Layer, NumLayers> layers;
    };

    MipmapTable();
    ~MipmapTable();

    // Call from message thread when waveform changes.
    // Joins any in-flight build before starting a new one.
    void setTable (const std::vector<float>& samples);

    // Call from audio thread — lock-free
    const float* getLayerForFreq (float freqHz, double sampleRate) const noexcept;

    // Returns the layer pointer for the given increment (freq * TableSize / sampleRate).
    // Use this at the start of a block to cache the pointer.
    const float* getLayerForIncrement (float increment) const noexcept;

    bool isReady() const noexcept { return activeIdx.load (std::memory_order_acquire) >= 0; }

private:
    std::array<TableSet, 2> tables;
    std::atomic<int>        activeIdx { -1 };  // -1 = not yet ready

    std::mutex  buildMutex;
    std::thread buildThread;   // joinable — never detached

    static void buildInto (TableSet& dest, const std::vector<float>& src);
    static void fillSaw   (std::vector<float>& buf);
};
