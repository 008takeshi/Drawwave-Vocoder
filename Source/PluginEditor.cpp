#include "PluginEditor.h"
#include "PluginProcessor.h"

DrawwaveVocoderEditor::DrawwaveVocoderEditor(DrawwaveVocoderProcessor& p)
    : AudioProcessorEditor(p), proc(p),
      bandGainEditor(proc.getVocoderCore().getNumBands()),
      knobPanel(proc.apvts),
      presetBrowser(p),
      pianoKeyboard(proc.keyboardState, proc.notesBits0, proc.notesBits1, 36, 84)
{
    setSize(1100, 680);
    setWantsKeyboardFocus(true);

    waveformEditor.setBuffer(proc.getWaveform());
    waveformEditor.addListener(this);
    addAndMakeVisible(waveformEditor);

    bandGainEditor.setVocoderCore(&proc.getVocoderCore());
    bandGainEditor.setFreqRange(80.0f, 11000.0f);
    bandGainEditor.addListener(this);
    addAndMakeVisible(bandGainEditor);

    lfoShaperEditor.setBuffer(proc.getLfoWaveform());
    lfoShaperEditor.addListener(this);
    addAndMakeVisible(lfoShaperEditor);

    addAndMakeVisible(knobPanel);
    addAndMakeVisible(presetBrowser);
    addAndMakeVisible(pianoKeyboard);

    presetBrowser.onPresetLoaded = [this]() { syncEditorsFromProcessor(); };

    startTimerHz(30);
}

DrawwaveVocoderEditor::~DrawwaveVocoderEditor()
{
    proc.keyboardState.allNotesOff(0);
    waveformEditor.removeListener(this);
    bandGainEditor.removeListener(this);
    lfoShaperEditor.removeListener(this);
    bandGainEditor.setVocoderCore(nullptr);
    stopTimer();
}

void DrawwaveVocoderEditor::visibilityChanged()
{
    if (isShowing() && proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        grabKeyboardFocus();
}

void DrawwaveVocoderEditor::resized()
{
    constexpr int lx = 10, lw = 500, lTop = 4, lGap = 8;
    constexpr int lh = (623 - lTop - lGap * 2) / 3;
    waveformEditor.setBounds  (lx, lTop,                   lw, lh);
    bandGainEditor.setBounds  (lx, lTop + lh + lGap,       lw, lh);
    lfoShaperEditor.setBounds (lx, lTop + (lh + lGap) * 2, lw, lh);

    knobPanel.setBounds(530, 4, 560, 370);
    presetBrowser.setBounds(530, 412, 560, 214);
    pianoKeyboard.setBounds(0, 632, getWidth(), 48);
}

void DrawwaveVocoderEditor::paintOverChildren(juce::Graphics& g)
{
    const float lfoDepth = proc.apvts.getRawParameterValue(ParamID::lfoDepth)->load();
    const float lfoRate  = proc.apvts.getRawParameterValue(ParamID::lfoRate)->load();
    const bool  lfoSync  = proc.apvts.getRawParameterValue(ParamID::lfoSync)->load() >= 0.5f;
    const int   lfoDest  = juce::jlimit(0, 2,
        (int)proc.apvts.getRawParameterValue(ParamID::lfoDest)->load());

    static const char* destNames[] = { "Formant", "Band Tilt", "Pitch" };

    const auto lfoR = lfoShaperEditor.getBounds();
    g.setColour(juce::Colour(0xff00d9ff));
    g.setFont(juce::FontOptions(10.0f));
    const juce::String info = juce::String::formatted(
        "%.2f Hz  depth %.0f%%  dest: %s%s",
        lfoRate, lfoDepth * 100.0f, destNames[lfoDest],
        lfoSync ? "  [SYNC]" : "");
    g.drawText(info, lfoR.getX(), lfoR.getY() + 4, lfoR.getWidth() - 6, 13,
               juce::Justification::right);
}

void DrawwaveVocoderEditor::timerCallback()
{
    const int coreBands = proc.getVocoderCore().getNumBands();
    if (bandGainEditor.getNumPoints() != coreBands)
    {
        bandGainEditor.setNumBands(coreBands);
        proc.setBandGains(bandGainEditor.getBuffer());
    }

    lfoShaperEditor.setPhase(proc.getLfo().getPhaseNormalized());
    repaint();
}

void DrawwaveVocoderEditor::syncEditorsFromProcessor()
{
    waveformEditor.setBuffer(proc.getWaveform());
    lfoShaperEditor.setBuffer(proc.getLfoWaveform());

    const int n = proc.getVocoderCore().getNumBands();
    if (bandGainEditor.getNumPoints() != n)
        bandGainEditor.setNumBands(n);
    std::vector<float> gains((size_t)n);
    for (int i = 0; i < n; ++i)
        gains[(size_t)i] = proc.getVocoderCore().getBandGain(i);
    bandGainEditor.setBuffer(gains);
}

void DrawwaveVocoderEditor::curveChanged(DrawableCurveComponent* comp)
{
    if (comp == &waveformEditor)
        proc.setWaveform(waveformEditor.getBuffer());
    else if (comp == &bandGainEditor)
        proc.setBandGains(bandGainEditor.getBuffer());
    else if (comp == &lfoShaperEditor)
        proc.setLfoWaveform(lfoShaperEditor.getBuffer());
}

void DrawwaveVocoderEditor::paint(juce::Graphics& g)
{
    const juce::Colour bg      { 0xff1a1a1f };
    const juce::Colour cyan    { 0xff00d9ff };
    const juce::Colour dimGrey { 0xff444455 };
    const juce::Colour green   { 0xff00ff88 };

    g.fillAll(bg);

    struct Meter { const char* label; float value; };
    const Meter meters[] = {
        { "MIC IN",  proc.modulatorRms.load(std::memory_order_relaxed) },
        { "CARRIER", proc.carrierPeak.load(std::memory_order_relaxed) },
        { "OUT",     proc.vocoderOutPeak.load(std::memory_order_relaxed) },
    };
    const float mx = (float)knobPanel.getX();
    const float mw = (float)knobPanel.getWidth();
    const float mh = 10.0f;
    const float bw = mw / 3.0f;

    g.setFont(juce::FontOptions(10.0f));
    for (int i = 0; i < 3; ++i)
    {
        const float x   = mx + (float)i * bw + 4.0f;
        const float w   = bw - 8.0f;
        const float y   = 384.0f;
        const float lvl = juce::jlimit(0.0f, 1.0f, meters[i].value * 4.0f);

        g.setColour(dimGrey);
        g.fillRect(x, y, w, mh);
        g.setColour(lvl > 0.001f ? green : dimGrey.brighter(0.3f));
        g.fillRect(x, y, w * lvl, mh);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawText(meters[i].label,
                   (int)x, (int)(y + mh + 2), (int)w, 12,
                   juce::Justification::centred);
    }
}

static const std::pair<int,int> kKeyMap[] = {
    { 'z', 48 }, { 's', 49 }, { 'x', 50 }, { 'd', 51 }, { 'c', 52 },
    { 'v', 53 }, { 'g', 54 }, { 'b', 55 }, { 'h', 56 }, { 'n', 57 },
    { 'j', 58 }, { 'm', 59 },
    { 'q', 60 }, { '2', 61 }, { 'w', 62 }, { '3', 63 }, { 'e', 64 },
    { 'r', 65 }, { '5', 66 }, { 't', 67 }, { '6', 68 }, { 'y', 69 },
    { '7', 70 }, { 'u', 71 },
    { 'i', 72 }, { '9', 73 }, { 'o', 74 }, { '0', 75 }, { 'p', 76 },
};

bool DrawwaveVocoderEditor::keyPressed(const juce::KeyPress& key)
{
    if (!key.getModifiers().isAnyModifierKeyDown())
        return true;
    return false;
}

bool DrawwaveVocoderEditor::keyStateChanged(bool)
{
    bool consumed = false;
    for (auto& [keyCode, note] : kKeyMap)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown(keyCode);
        const bool held = keysHeld.count(keyCode) > 0;

        if (down && !held)
        {
            proc.keyboardState.noteOn(1, note, 0.8f);
            keysHeld.insert(keyCode);
            consumed = true;
        }
        else if (!down && held)
        {
            proc.keyboardState.noteOff(1, note, 0.0f);
            keysHeld.erase(keyCode);
            consumed = true;
        }
    }
    return consumed;
}
