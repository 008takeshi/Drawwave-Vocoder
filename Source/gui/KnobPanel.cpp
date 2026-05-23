#include "KnobPanel.h"
#include "../state/Parameters.h"

KnobPanel::KnobPanel(juce::AudioProcessorValueTreeState& a)
    : apvts(a), standalone(juce::JUCEApplicationBase::isStandaloneApp())
{
    // ---- LFO ----
    styleSlider(lfoRateSlider);
    styleSlider(lfoDepthSlider);
    styleToggle(lfoSyncBtn);
    styleToggle(lfoRetrigBtn);

    lfoDestBox.addItem("Formant",  1);
    lfoDestBox.addItem("Tilt",     2);
    lfoDestBox.addItem("Pitch",    3);

    lfoSyncDivBox.addItem("4/1",   1);
    lfoSyncDivBox.addItem("2/1",   2);
    lfoSyncDivBox.addItem("1/1",   3);
    lfoSyncDivBox.addItem("1/2",   4);
    lfoSyncDivBox.addItem("1/4",   5);
    lfoSyncDivBox.addItem("1/8",   6);
    lfoSyncDivBox.addItem("1/16",  7);
    lfoSyncDivBox.addItem("1/32",  8);

    addAndMakeVisible(lfoRateSlider);
    addAndMakeVisible(lfoDepthSlider);
    addAndMakeVisible(lfoDestBox);
    addAndMakeVisible(lfoRetrigBtn);
    addAndMakeVisible(lfoSyncBtn);
    addAndMakeVisible(lfoSyncDivBox);

    lfoSyncBtn.setVisible(!standalone);
    lfoSyncDivBox.setVisible(!standalone);

    // ---- Vocoder ----
    styleSlider(micGainSlider);
    styleSlider(formantSlider);
    styleSlider(envAttackSlider);
    styleSlider(envReleaseSlider);

    bandCountBox.addItem("8",  1);
    bandCountBox.addItem("16", 2);
    bandCountBox.addItem("24", 3);
    bandCountBox.addItem("32", 4);

    addAndMakeVisible(micGainSlider);
    addAndMakeVisible(formantSlider);
    addAndMakeVisible(envAttackSlider);
    addAndMakeVisible(envReleaseSlider);
    addAndMakeVisible(bandCountBox);

    // ---- Output ----
    styleSlider(wetDrySlider);
    styleSlider(carrierMixSlider);
    styleSlider(hpFreqSlider);
    styleSlider(lpFreqSlider);
    styleSlider(masterGainSlider);
    styleSlider(noiseMixSlider);

    addAndMakeVisible(wetDrySlider);
    addAndMakeVisible(carrierMixSlider);
    addAndMakeVisible(hpFreqSlider);
    addAndMakeVisible(lpFreqSlider);
    addAndMakeVisible(masterGainSlider);
    addAndMakeVisible(noiseMixSlider);

    // ---- Attachments (after ComboBox items are populated) ----
    lfoRateAtt    = std::make_unique<SliderAtt>(apvts, ParamID::lfoRate,     lfoRateSlider);
    lfoDepthAtt   = std::make_unique<SliderAtt>(apvts, ParamID::lfoDepth,    lfoDepthSlider);
    lfoSyncAtt    = std::make_unique<ButtonAtt>(apvts, ParamID::lfoSync,     lfoSyncBtn);
    lfoRetrigAtt  = std::make_unique<ButtonAtt>(apvts, ParamID::lfoRetrigger,lfoRetrigBtn);
    lfoDestAtt    = std::make_unique<ComboAtt> (apvts, ParamID::lfoDest,     lfoDestBox);
    lfoSyncDivAtt = std::make_unique<ComboAtt> (apvts, ParamID::lfoSyncDiv,  lfoSyncDivBox);

    micGainAtt     = std::make_unique<SliderAtt>(apvts, ParamID::micGain,     micGainSlider);
    formantAtt     = std::make_unique<SliderAtt>(apvts, ParamID::formantShift, formantSlider);
    envAttackAtt   = std::make_unique<SliderAtt>(apvts, ParamID::envAttack,    envAttackSlider);
    envReleaseAtt  = std::make_unique<SliderAtt>(apvts, ParamID::envRelease,   envReleaseSlider);
    bandCountAtt   = std::make_unique<ComboAtt> (apvts, ParamID::bandCount,    bandCountBox);

    wetDryAtt      = std::make_unique<SliderAtt>(apvts, ParamID::wetDry,       wetDrySlider);
    carrierMixAtt  = std::make_unique<SliderAtt>(apvts, ParamID::carrierMix,   carrierMixSlider);
    hpFreqAtt      = std::make_unique<SliderAtt>(apvts, ParamID::hpFreq,       hpFreqSlider);
    lpFreqAtt      = std::make_unique<SliderAtt>(apvts, ParamID::lpFreq,       lpFreqSlider);
    masterGainAtt  = std::make_unique<SliderAtt>(apvts, ParamID::masterGain,   masterGainSlider);
    noiseMixAtt    = std::make_unique<SliderAtt>(apvts, ParamID::noiseMix,     noiseMixSlider);
}

KnobPanel::~KnobPanel()
{
    // Attachments are destroyed before sliders via member declaration order
}

void KnobPanel::styleSlider(juce::Slider& s)
{
    s.setColour(juce::Slider::rotarySliderFillColourId,  juce::Colour(0xff00d9ff));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333344));
    s.setColour(juce::Slider::thumbColourId,             juce::Colour(0xff00d9ff));
    s.setColour(juce::Slider::textBoxTextColourId,       juce::Colour(0xff888899));
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111122));
    s.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 14);
}

void KnobPanel::styleToggle(juce::ToggleButton& b)
{
    b.setColour(juce::ToggleButton::textColourId,      juce::Colour(0xff888899));
    b.setColour(juce::ToggleButton::tickColourId,      juce::Colour(0xff00d9ff));
    b.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff444455));
}

void KnobPanel::paint(juce::Graphics& g)
{
    const juce::Colour bg     { 0xff1e1e28 };
    const juce::Colour border { 0xff333344 };

    g.fillAll(bg);
    g.setColour(border);
    g.drawRect(getLocalBounds().toFloat(), 1.0f);

    const float cw      = (float)getWidth() / 3.0f;
    const int   headerH = 18;
    const int   cellH   = (getHeight() - headerH) / 3;
    constexpr int kSz   = 64;

    auto cellTopLblY = [&](int row) { return headerH + cellH * row + 3; };

    // Section labels
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(juce::Colour(0xff666677));
    const char* secNames[] = { "LFO", "VOCODER", "OUTPUT" };
    for (int i = 0; i < 3; ++i)
        g.drawText(secNames[i], (int)(cw * (float)i + 4), 4, (int)cw - 8, 12,
                   juce::Justification::left);

    // Dividers
    g.setColour(border);
    g.drawVerticalLine((int)cw,     4.0f, (float)getHeight() - 4.0f);
    g.drawVerticalLine((int)(2*cw), 4.0f, (float)getHeight() - 4.0f);

    // Control labels — size 11, bright, centred at knob x
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colour(0xff9999aa));

    auto lbl = [&](float cx, int y, const char* text)
    {
        g.drawText(text, (int)(cx - 24.0f), y, 48, 13, juce::Justification::centred);
    };

    // LFO col
    lbl(cw * 0.28f, cellTopLblY(0), "Rate");
    lbl(cw * 0.72f, cellTopLblY(0), "Depth");
    lbl(cw * 0.28f, cellTopLblY(1), "Dest");
    lbl(cw * 0.72f, cellTopLblY(1), "Retrig");
    if (!standalone)
    {
        lbl(cw * 0.28f, cellTopLblY(2), "Sync");
        lbl(cw * 0.72f, cellTopLblY(2), "Div");
    }

    // Vocoder col
    lbl(cw * 1.28f, cellTopLblY(0), "Mic Gain");
    lbl(cw * 1.72f, cellTopLblY(0), "Frmt");
    lbl(cw * 1.28f, cellTopLblY(1), "Atk");
    lbl(cw * 1.72f, cellTopLblY(1), "Rel");
    lbl(cw * 1.50f, cellTopLblY(2), "Bands");

    // Output col
    lbl(cw * 2.28f, cellTopLblY(0), "Mic Mix");
    lbl(cw * 2.72f, cellTopLblY(0), "Car Mix");
    lbl(cw * 2.28f, cellTopLblY(1), "HP");
    lbl(cw * 2.72f, cellTopLblY(1), "LP");
    lbl(cw * 2.28f, cellTopLblY(2), "Gain");
    lbl(cw * 2.72f, cellTopLblY(2), "Noise");
}

void KnobPanel::resized()
{
    constexpr int kSz     = 64;
    constexpr int headerH = 18;
    const int     cellH   = (getHeight() - headerH) / 3;
    const float   cw      = (float)getWidth() / 3.0f;

    // Each control is centred vertically within its grid cell
    auto cellCY = [&](int row) { return headerH + cellH * row + cellH / 2; };

    auto placeKnob = [&](juce::Slider& s, float cx, int row)
    {
        const int cy = cellCY(row);
        s.setBounds((int)(cx - kSz * 0.5f), cy - (kSz + 16) / 2, kSz, kSz + 16);
    };
    auto placeCombo = [&](juce::ComboBox& c, float cx, int row, int cbw = 62)
    {
        const int cy = cellCY(row);
        c.setBounds((int)(cx - (float)cbw * 0.5f), cy - 10, cbw, 20);
    };
    auto placeToggle = [&](juce::ToggleButton& b, float cx, int row)
    {
        const int cy = cellCY(row);
        b.setBounds((int)(cx - 28), cy - 9, 56, 18);
    };

    // LFO (col 0)
    placeKnob  (lfoRateSlider,  cw * 0.28f, 0);
    placeKnob  (lfoDepthSlider, cw * 0.72f, 0);
    placeCombo (lfoDestBox,     cw * 0.28f, 1);
    placeToggle(lfoRetrigBtn,   cw * 0.72f, 1);
    placeToggle(lfoSyncBtn,     cw * 0.28f, 2);
    placeCombo (lfoSyncDivBox,  cw * 0.72f, 2);

    // Vocoder (col 1) — Mic first
    placeKnob  (micGainSlider,    cw * 1.28f, 0);
    placeKnob  (formantSlider,    cw * 1.72f, 0);
    placeKnob  (envAttackSlider,  cw * 1.28f, 1);
    placeKnob  (envReleaseSlider, cw * 1.72f, 1);
    placeCombo (bandCountBox,     cw * 1.50f, 2);

    // Output (col 2)
    placeKnob  (wetDrySlider,     cw * 2.28f, 0);
    placeKnob  (carrierMixSlider, cw * 2.72f, 0);
    placeKnob  (hpFreqSlider,     cw * 2.28f, 1);
    placeKnob  (lpFreqSlider,     cw * 2.72f, 1);
    placeKnob  (masterGainSlider, cw * 2.28f, 2);
    placeKnob  (noiseMixSlider,   cw * 2.72f, 2);
}
