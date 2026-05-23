#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

static int bandChoiceToCount(int idx);

DrawwaveVocoderProcessor::DrawwaveVocoderProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output",    juce::AudioChannelSet::stereo(), true)
        .withInput ("Modulator", juce::AudioChannelSet::stereo(), true)
        .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Default carrier waveform: Saw
    waveformSamples.resize(MipmapTable::TableSize);
    for (int i = 0; i < MipmapTable::TableSize; ++i)
        waveformSamples[(size_t)i] = 2.0f * (float)i / (float)MipmapTable::TableSize - 1.0f;

    // Default LFO waveform: Sine (matches DrawableLfo constructor)
    lfoWaveformSamples.resize(DrawableLfo::TableSize);
    for (int i = 0; i < DrawableLfo::TableSize; ++i)
        lfoWaveformSamples[(size_t)i] = std::sin(
            juce::MathConstants<float>::twoPi * (float)i / (float)DrawableLfo::TableSize);

    voiceManager.setTable(&mipmapTable);

    // Only Q changes go through parameterChanged; formant shift is now applied
    // every block on the audio thread via applyLfoFormantShift.
    apvts.addParameterListener(ParamID::vocoderQ,    this);
    apvts.addParameterListener(ParamID::bandCount,   this);
}

DrawwaveVocoderProcessor::~DrawwaveVocoderProcessor()
{
    apvts.removeParameterListener(ParamID::vocoderQ,  this);
    apvts.removeParameterListener(ParamID::bandCount,  this);
}

void DrawwaveVocoderProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (paramID == ParamID::vocoderQ)
        vocoderCore.setQ(newValue);
    else if (paramID == ParamID::bandCount)
        vocoderCore.setBandCount(bandChoiceToCount((int)newValue));
}

bool DrawwaveVocoderProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto& mainIn = layouts.getMainInputChannelSet();
    if (mainIn.isDisabled())
        return false;
    if (mainIn != juce::AudioChannelSet::stereo() && mainIn != juce::AudioChannelSet::mono())
        return false;

    const auto& sc = layouts.getChannelSet(true, 1);
    if (!sc.isDisabled() && sc != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

static int bandChoiceToCount(int idx)
{
    static const int counts[] = { 8, 16, 24, 32 };
    return counts[juce::jlimit(0, 3, idx)];
}

void DrawwaveVocoderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
    voiceManager.setTable(&mipmapTable);

    const int bands = bandChoiceToCount(
        (int)apvts.getRawParameterValue(ParamID::bandCount)->load());
    const float q   = apvts.getRawParameterValue(ParamID::vocoderQ)->load();
    const float atk = apvts.getRawParameterValue(ParamID::envAttack)->load();
    const float rel = apvts.getRawParameterValue(ParamID::envRelease)->load();

    vocoderCore.prepare(sampleRate, samplesPerBlock,
                        bands, 80.0f, 11000.0f, q, atk, rel);

    lfo.prepare(sampleRate);
    outputStage.prepare(sampleRate, samplesPerBlock);
    unvoicedDetector.prepare(sampleRate);

    carrierMono.assign((size_t)samplesPerBlock, 0.0f);
    modulatorMono.assign((size_t)samplesPerBlock, 0.0f);
    modulatorDry.assign((size_t)samplesPerBlock, 0.0f);
    vocoderOut.assign((size_t)samplesPerBlock, 0.0f);
}

void DrawwaveVocoderProcessor::setBandGains(const std::vector<float>& gains)
{
    const int limit = std::min({ (int)gains.size(),
                                 VocoderCore::MaxBands,
                                 vocoderCore.getNumBands() });
    for (int i = 0; i < limit; ++i)
        vocoderCore.setBandGain(i, gains[(size_t)i]);
}

void DrawwaveVocoderProcessor::setWaveform(const std::vector<float>& samples)
{
    waveformSamples = samples;
    waveformSamples.resize(MipmapTable::TableSize, 0.0f);
    mipmapTable.setTable(waveformSamples);
}

void DrawwaveVocoderProcessor::setLfoWaveform(const std::vector<float>& samples)
{
    lfoWaveformSamples = samples;
    lfoWaveformSamples.resize(DrawableLfo::TableSize, 0.0f);
    lfo.setTable(lfoWaveformSamples);
}

void DrawwaveVocoderProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    keyboardState.processNextMidiBuffer(midi, 0, numSamples, true);

    // ADSR sync
    {
        auto msToSec = [](float ms) { return ms * 0.001f; };
        juce::ADSR::Parameters adsrParams;
        adsrParams.attack  = msToSec(apvts.getRawParameterValue(ParamID::ampAttack)->load());
        adsrParams.decay   = msToSec(apvts.getRawParameterValue(ParamID::ampDecay)->load());
        adsrParams.sustain = apvts.getRawParameterValue(ParamID::ampSustain)->load();
        adsrParams.release = msToSec(apvts.getRawParameterValue(ParamID::ampRelease)->load());
        voiceManager.setAdsrParams(adsrParams);
    }

    // Vocoder envelope sync
    vocoderCore.setAttackMs (apvts.getRawParameterValue(ParamID::envAttack)->load());
    vocoderCore.setReleaseMs(apvts.getRawParameterValue(ParamID::envRelease)->load());

    // ---- LFO ----
    {
        // BPM from host (graceful fallback to 120)
        double bpm = 120.0;
        if (auto* head = getPlayHead())
            if (auto pos = head->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;

        const float lfoRateHz  = apvts.getRawParameterValue(ParamID::lfoRate)->load();
        const bool  lfoSync    = apvts.getRawParameterValue(ParamID::lfoSync)->load() >= 0.5f;
        const int   syncDivIdx = juce::jlimit(0, 7,
            (int)apvts.getRawParameterValue(ParamID::lfoSyncDiv)->load());
        const float lfoDepth   = apvts.getRawParameterValue(ParamID::lfoDepth)->load();
        const int   lfoDest    = juce::jlimit(0, 2,
            (int)apvts.getRawParameterValue(ParamID::lfoDest)->load());
        const bool  lfoRetrig  = apvts.getRawParameterValue(ParamID::lfoRetrigger)->load() >= 0.5f;

        static constexpr float kSyncBeats[] =
            { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };
        const float syncBeats = kSyncBeats[syncDivIdx];

        // Check for noteOn retrigger before advancing
        if (lfoRetrig)
        {
            for (const auto& meta : midi)
                if (meta.getMessage().isNoteOn()) { lfo.retrigger(); break; }
        }

        lfo.prepareBlock(lfoRateHz, lfoSync, syncBeats, bpm, numSamples);
        const float lfoMod = lfo.getOutput() * lfoDepth;

        // Static formant shift read directly on audio thread (safe: atomic float*)
        const float staticShift = apvts.getRawParameterValue(ParamID::formantShift)->load();

        switch (lfoDest)
        {
            case 0:  // Formant Shift
                vocoderCore.applyLfoFormantShift(staticShift + lfoMod * 12.0f);
                vocoderCore.setBandGainTiltOffset(0.0f);
                voiceManager.setPitchOffsetCents(0.0f);
                break;
            case 1:  // Band Gain Tilt
                vocoderCore.applyLfoFormantShift(staticShift);
                vocoderCore.setBandGainTiltOffset(lfoMod);
                voiceManager.setPitchOffsetCents(0.0f);
                break;
            case 2:  // Carrier Pitch
                vocoderCore.applyLfoFormantShift(staticShift);
                vocoderCore.setBandGainTiltOffset(0.0f);
                voiceManager.setPitchOffsetCents(lfoMod * 100.0f);
                break;
            default: break;
        }
    }

    // ---- Modulator ----
    static constexpr float kModulatorPreGain = 20.0f;

    bool hasModulator = false;
    {
        std::fill(modulatorMono.begin(), modulatorMono.begin() + numSamples, 0.0f);

        auto scBus     = getBusBuffer(buffer, true, 1);
        auto mainInBus = getBusBuffer(buffer, true, 0);
        const auto& activeBus = (scBus.getNumChannels() > 0) ? scBus : mainInBus;

        modulatorRms.store(0.0f, std::memory_order_relaxed);

        if (activeBus.getNumChannels() > 0)
        {
            const int   nCh    = activeBus.getNumChannels();
            const float inv    = 1.0f / (float)nCh;
            float       rmsSum = 0.0f;

            for (int ch = 0; ch < nCh; ++ch)
            {
                const float* src = activeBus.getReadPointer(ch);
                for (int s = 0; s < numSamples; ++s)
                {
                    modulatorMono[(size_t)s] += src[s] * inv;
                    rmsSum += src[s] * src[s];
                }
            }

            hasModulator = (rmsSum > 1e-10f);
            modulatorRms.store(std::sqrt(rmsSum / (float)(nCh * numSamples)),
                               std::memory_order_relaxed);

            if (hasModulator)
            {
                std::copy(modulatorMono.begin(), modulatorMono.begin() + numSamples,
                          modulatorDry.begin());

                const float micGainLin = juce::Decibels::decibelsToGain(
                    apvts.getRawParameterValue(ParamID::micGain)->load());
                for (int s = 0; s < numSamples; ++s)
                    modulatorMono[(size_t)s] *= kModulatorPreGain * micGainLin;
            }
            else
            {
                std::fill(modulatorDry.begin(), modulatorDry.begin() + numSamples, 0.0f);
            }
        }

        // Unvoiced detection: inject noise for sibilants / silence
        const float noiseSens = apvts.getRawParameterValue(ParamID::noiseSensitivity)->load();
        const float noiseMix  = apvts.getRawParameterValue(ParamID::noiseMix)->load();
        if (hasModulator && noiseMix > 1e-4f)
        {
            unvoicedDetector.setSensitivity(noiseSens);
            const bool voiced = unvoicedDetector.process(modulatorMono.data(), numSamples);
            if (!voiced)
                for (int s = 0; s < numSamples; ++s)
                    modulatorMono[(size_t)s] += (rng.nextFloat() * 2.0f - 1.0f)
                                                * noiseMix * 0.1f;
        }
    }

    // ---- Carrier ----
    {
        // Scan MIDI for GUI note display
        {
            uint64_t b0 = notesBits0.load(std::memory_order_relaxed);
            uint64_t b1 = notesBits1.load(std::memory_order_relaxed);
            for (const auto& meta : midi)
            {
                const auto msg = meta.getMessage();
                const int  n   = msg.getNoteNumber();
                if (n >= 0 && n < 128)
                {
                    if (msg.isNoteOn())
                    {
                        if (n < 64) b0 |=  (1ULL << n);
                        else        b1 |=  (1ULL << (n - 64));
                    }
                    else if (msg.isNoteOff())
                    {
                        if (n < 64) b0 &= ~(1ULL << n);
                        else        b1 &= ~(1ULL << (n - 64));
                    }
                    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                    {
                        b0 = 0; b1 = 0;
                    }
                }
            }
            notesBits0.store(b0, std::memory_order_relaxed);
            notesBits1.store(b1, std::memory_order_relaxed);
        }

        buffer.clear();
        voiceManager.process(midi, buffer);

        std::fill(carrierMono.begin(), carrierMono.begin() + numSamples, 0.0f);
        const int   nCh = buffer.getNumChannels();
        const float inv = (nCh > 0) ? 1.0f / (float)nCh : 1.0f;
        for (int ch = 0; ch < nCh; ++ch)
        {
            const float* src = buffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s)
                carrierMono[(size_t)s] += src[s] * inv;
        }

        float cpeak = 0.0f;
        for (int s = 0; s < numSamples; ++s)
            cpeak = std::max(cpeak, std::abs(carrierMono[(size_t)s]));
        carrierPeak.store(cpeak, std::memory_order_relaxed);
    }

    // ---- Vocoder ----
    vocoderCore.process(carrierMono.data(), modulatorMono.data(),
                        vocoderOut.data(), numSamples);

    // ---- OutputStage (HP/LP + tanh soft-clip) ----
    {
        const float hpHz = apvts.getRawParameterValue(ParamID::hpFreq)->load();
        const float lpHz = apvts.getRawParameterValue(ParamID::lpFreq)->load();
        outputStage.process(vocoderOut.data(), numSamples, hpHz, lpHz);
    }

    {
        float opeak = 0.0f;
        for (int s = 0; s < numSamples; ++s)
            opeak = std::max(opeak, std::abs(vocoderOut[(size_t)s]));
        vocoderOutPeak.store(opeak, std::memory_order_relaxed);
    }

    // ---- Output mix: vocoder + mic blend + carrier ----
    {
        const float micBlend   = hasModulator
                                 ? apvts.getRawParameterValue(ParamID::wetDry)->load()
                                 : 0.0f;
        const float carrierAmt = apvts.getRawParameterValue(ParamID::carrierMix)->load();

        const float modEnv     = vocoderCore.getModulatorEnv();
        const float carrierGain = modEnv > 0.025f ? (modEnv - 0.025f) : 0.0f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* dst = buffer.getWritePointer(ch);
            for (int s = 0; s < numSamples; ++s)
                dst[s] = vocoderOut[(size_t)s]
                       + micBlend  * modulatorDry[(size_t)s]
                       + carrierAmt * carrierMono[(size_t)s] * carrierGain;
        }
    }

    // Master gain
    const float gainDb = apvts.getRawParameterValue(ParamID::masterGain)->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(gainDb));
}

juce::AudioProcessorEditor* DrawwaveVocoderProcessor::createEditor()
{
    return new DrawwaveVocoderEditor(*this);
}

void DrawwaveVocoderProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();

    // Carrier waveform
    juce::MemoryBlock waveBlock(waveformSamples.size() * sizeof(float));
    std::memcpy(waveBlock.getData(), waveformSamples.data(), waveBlock.getSize());
    state.setProperty("waveform", waveBlock, nullptr);

    // LFO waveform
    juce::MemoryBlock lfoBlock(lfoWaveformSamples.size() * sizeof(float));
    std::memcpy(lfoBlock.getData(), lfoWaveformSamples.data(), lfoBlock.getSize());
    state.setProperty("lfoWaveform", lfoBlock, nullptr);

    // Band gains
    juce::MemoryBlock gainBlock(VocoderCore::MaxBands * sizeof(float));
    float* gainPtr = static_cast<float*>(gainBlock.getData());
    for (int i = 0; i < VocoderCore::MaxBands; ++i)
        gainPtr[i] = vocoderCore.getBandGain(i);
    state.setProperty("bandGains", gainBlock, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void DrawwaveVocoderProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);

        // Band gains
        if (auto* prop = state.getPropertyPointer("bandGains"))
        {
            if (auto* mb = prop->getBinaryData())
            {
                const int n = (int)(mb->getSize() / sizeof(float));
                const float* ptr = static_cast<const float*>(mb->getData());
                vocoderCore.resetBandGains();
                for (int i = 0; i < n && i < VocoderCore::MaxBands; ++i)
                    vocoderCore.setBandGain(i, ptr[i]);
            }
        }

        // Carrier waveform
        if (auto* prop = state.getPropertyPointer("waveform"))
        {
            if (auto* mb = prop->getBinaryData())
            {
                const size_t n = mb->getSize() / sizeof(float);
                std::vector<float> buf(n);
                std::memcpy(buf.data(), mb->getData(), mb->getSize());
                setWaveform(buf);
            }
        }

        // LFO waveform
        if (auto* prop = state.getPropertyPointer("lfoWaveform"))
        {
            if (auto* mb = prop->getBinaryData())
            {
                const size_t n = mb->getSize() / sizeof(float);
                std::vector<float> buf(n);
                std::memcpy(buf.data(), mb->getData(), mb->getSize());
                setLfoWaveform(buf);
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrawwaveVocoderProcessor();
}
