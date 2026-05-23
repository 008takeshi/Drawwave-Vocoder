#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "MipmapTable.h"
#include "WavetableOscillator.h"

class WtVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound*) override { return true; }

    void startNote  (int midiNote, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote   (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

    void prepareVoice     (double sampleRate);
    void setAdsrParameters(const juce::ADSR::Parameters& p) { adsr.setParameters(p); }
    void setTable         (const MipmapTable* t) noexcept { osc.setTable(t); }

    // Audio thread: shift frequency by semitones offset from the base MIDI pitch.
    void setFreqOffset(float semitones) noexcept;

private:
    WavetableOscillator osc;
    juce::ADSR          adsr;
    float               level       = 1.0f;
    double              sampleRate  = 44100.0;
    float               baseFreqHz  = 440.0f;
};

class WtSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class VoiceManager
{
public:
    VoiceManager();

    void prepare      (double sampleRate, int samplesPerBlock);
    void process      (juce::MidiBuffer& midi, juce::AudioBuffer<float>& audio);
    void setAdsrParams(const juce::ADSR::Parameters& p);
    void setTable     (const MipmapTable* t) noexcept;

    // Audio thread: shift all active voices by centOffset cents (100 cents = 1 semitone).
    void setPitchOffsetCents(float cents) noexcept;

    static constexpr int MaxVoices = 8;

private:
    juce::Synthesiser       synth;
    juce::ADSR::Parameters  adsrParams;
    double                  sampleRate = 44100.0;
    WtVoice*                voicePtrs[MaxVoices] {};
};
