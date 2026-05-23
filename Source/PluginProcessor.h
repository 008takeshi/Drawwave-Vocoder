#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "state/Parameters.h"
#include "dsp/MipmapTable.h"
#include "dsp/VoiceManager.h"
#include "dsp/VocoderCore.h"
#include "dsp/DrawableLfo.h"
#include "dsp/OutputStage.h"
#include "dsp/UnvoicedDetector.h"

class DrawwaveVocoderProcessor : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    DrawwaveVocoderProcessor();
    ~DrawwaveVocoderProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    void parameterChanged(const juce::String& paramID, float newValue) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Waveform (called from message thread via WaveformEditor listener)
    void setWaveform(const std::vector<float>& samples);
    const std::vector<float>& getWaveform() const noexcept { return waveformSamples; }

    // LFO waveform (called from message thread via LfoShaperEditor listener)
    void setLfoWaveform(const std::vector<float>& samples);
    const std::vector<float>& getLfoWaveform() const noexcept { return lfoWaveformSamples; }
    DrawableLfo& getLfo() noexcept { return lfo; }

    // Band gains (called from message thread via BandGainEditor listener)
    void setBandGains(const std::vector<float>& gains);

    juce::AudioProcessorValueTreeState apvts;
    VocoderCore&       getVocoderCore()       noexcept { return vocoderCore; }
    const VocoderCore& getVocoderCore() const noexcept { return vocoderCore; }

    juce::MidiKeyboardState keyboardState;

    // GUI diagnostics (written by audio thread, read by GUI thread)
    std::atomic<float>    modulatorRms   { 0.0f };
    std::atomic<float>    carrierPeak    { 0.0f };
    std::atomic<float>    vocoderOutPeak { 0.0f };
    std::atomic<uint64_t> notesBits0     { 0 };
    std::atomic<uint64_t> notesBits1     { 0 };

private:
    MipmapTable      mipmapTable;
    VoiceManager     voiceManager;
    VocoderCore      vocoderCore;
    DrawableLfo      lfo;
    OutputStage      outputStage;
    UnvoicedDetector unvoicedDetector;
    juce::Random     rng;  // audio-thread-only noise generator

    std::vector<float> waveformSamples;
    std::vector<float> lfoWaveformSamples;

    std::vector<float> carrierMono;
    std::vector<float> modulatorMono;
    std::vector<float> modulatorDry;
    std::vector<float> vocoderOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrawwaveVocoderProcessor)
};
