#include "VoiceManager.h"

// ---------------------------------------------------------------------------
// WtVoice
// ---------------------------------------------------------------------------

void WtVoice::prepareVoice(double sr)
{
    sampleRate = sr;
    adsr.setSampleRate(sr);
}

void WtVoice::startNote(int midiNote, float velocity, juce::SynthesiserSound*, int)
{
    baseFreqHz = (float)juce::MidiMessage::getMidiNoteInHertz(midiNote);
    osc.setFrequency(baseFreqHz, sampleRate);
    osc.reset();
    level = velocity;
    adsr.noteOn();
}

void WtVoice::setFreqOffset(float semitones) noexcept
{
    osc.setFrequency(baseFreqHz * std::pow(2.0f, semitones / 12.0f), sampleRate);
}

void WtVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff) adsr.noteOff();
    else { clearCurrentNote(); adsr.reset(); }
}

void WtVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                               int startSample, int numSamples)
{
    if (!isVoiceActive()) return;

    osc.prepareBlock();

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        const float sample = osc.processSample() * level * adsr.getNextSample();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, i, sample);

        if (!adsr.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// VoiceManager
// ---------------------------------------------------------------------------

VoiceManager::VoiceManager()
{
    synth.addSound(new WtSound());
    for (int i = 0; i < MaxVoices; ++i)
    {
        auto* v = new WtVoice();
        voicePtrs[i] = v;
        synth.addVoice(v);
    }

    adsrParams.attack  = 0.005f;
    adsrParams.decay   = 0.1f;
    adsrParams.sustain = 0.8f;
    adsrParams.release = 0.2f;
}

void VoiceManager::prepare(double sr, int)
{
    sampleRate = sr;
    synth.setCurrentPlaybackSampleRate(sr);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<WtVoice*>(synth.getVoice(i)))
            v->prepareVoice(sr);
}

void VoiceManager::process(juce::MidiBuffer& midi, juce::AudioBuffer<float>& audio)
{
    synth.renderNextBlock(audio, midi, 0, audio.getNumSamples());
}

void VoiceManager::setAdsrParams(const juce::ADSR::Parameters& p)
{
    for (int i = 0; i < MaxVoices; ++i)
        voicePtrs[i]->setAdsrParameters(p);
}

void VoiceManager::setTable(const MipmapTable* t) noexcept
{
    for (int i = 0; i < MaxVoices; ++i)
        voicePtrs[i]->setTable(t);
}

void VoiceManager::setPitchOffsetCents(float cents) noexcept
{
    const float semitones = cents / 100.0f;
    for (int i = 0; i < MaxVoices; ++i)
        if (voicePtrs[i]->isVoiceActive())
            voicePtrs[i]->setFreqOffset(semitones);
}
