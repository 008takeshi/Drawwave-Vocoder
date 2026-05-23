#pragma once
#include "FilterBank.h"
#include "EnvelopeFollower.h"
#include <array>
#include <atomic>

class VocoderCore
{
public:
    static constexpr int MaxBands = FilterBank::MaxBands;

    VocoderCore();

    void prepare(double sampleRate, int samplesPerBlock,
                 int bandCount, float freqLow, float freqHigh,
                 float q, float attackMs, float releaseMs);

    void process(const float* carrier, const float* modulator,
                 float* output, int numSamples);

    void setBandCount(int n);
    void setQ(float q);
    void setAttackMs(float ms);
    void setReleaseMs(float ms);

    // Audio thread: apply formant shift + optional LFO modulation to synth bank.
    // Called every block from PluginProcessor::processBlock.
    void applyLfoFormantShift(float totalSemitones) noexcept;

    // Audio thread: set band gain tilt amount [-1, 1]; applied in process().
    void setBandGainTiltOffset(float amount) noexcept { tiltOffset = amount; }

    float getBandLevel(int band) const noexcept;
    float getModulatorEnv()      const noexcept;
    int   getNumBands()          const noexcept
        { return numBands.load(std::memory_order_acquire); }

    void  setBandGain  (int band, float gain);
    float getBandGain  (int band) const noexcept;
    void  resetBandGains();

private:
    void rebuildBanks();

    double           sampleRate     = 44100.0;
    int              samplesPerBlock = 512;
    std::atomic<int> numBands       { 16 };
    float  freqLow  = 80.0f;
    float  freqHigh = 11000.0f;
    float  q        = 4.0f;
    float  attackMs = 5.0f;
    float  releaseMs = 50.0f;

    float tiltOffset = 0.0f;  // audio thread only

    FilterBank analyzeBank;
    FilterBank synthBank;

    std::array<EnvelopeFollower, MaxBands> envFollowers;
    std::array<std::atomic<float>, MaxBands> bandLevels;
    std::array<std::atomic<float>, MaxBands> bandGains;

    std::vector<std::vector<float>> analyzeBands;
    std::vector<std::vector<float>> synthBands;
};
