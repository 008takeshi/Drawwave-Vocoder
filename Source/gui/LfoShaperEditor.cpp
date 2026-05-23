#include "LfoShaperEditor.h"
#include "../dsp/DrawableLfo.h"
#include <cmath>

LfoShaperEditor::LfoShaperEditor()
    : DrawableCurveComponent(DrawableLfo::TableSize, -1.0f, 1.0f)
{
    setupButton(btnSine, "Sine");
    setupButton(btnTri,  "Tri");
    setupButton(btnRamp, "Ramp");
    setupButton(btnRand, "Rand");

    btnSine.onClick = [this]{ loadPreset(Sine); };
    btnTri.onClick  = [this]{ loadPreset(Triangle); };
    btnRamp.onClick = [this]{ loadPreset(Ramp); };
    btnRand.onClick = [this]{ loadPreset(Random); };

    loadPreset(Sine);
}

void LfoShaperEditor::setupButton(juce::TextButton& btn, const juce::String& label)
{
    btn.setButtonText(label);
    btn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff222233));
    btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d9ff));
    btn.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xff888899));
    btn.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xff1a1a1f));
    addAndMakeVisible(btn);
}

void LfoShaperEditor::resized()
{
    auto area  = getLocalBounds();
    auto strip = area.removeFromBottom(24);
    const int bw = strip.getWidth() / 4;
    btnSine.setBounds(strip.removeFromLeft(bw));
    btnTri.setBounds (strip.removeFromLeft(bw));
    btnRamp.setBounds(strip.removeFromLeft(bw));
    btnRand.setBounds(strip);
}

void LfoShaperEditor::loadPreset(PresetShape shape)
{
    const int N = DrawableLfo::TableSize;
    buffer.resize((size_t)N);

    juce::Random rng(42);
    for (int i = 0; i < N; ++i)
    {
        const float t = (float)i / (float)N;
        float v = 0.0f;
        switch (shape)
        {
            case Sine:
                v = std::sin(juce::MathConstants<float>::twoPi * t);
                break;
            case Triangle:
                v = 1.0f - 4.0f * std::abs(t - 0.5f);
                break;
            case Ramp:
                v = 2.0f * t - 1.0f;
                break;
            case Random:
                v = rng.nextFloat() * 2.0f - 1.0f;
                break;
        }
        buffer[(size_t)i] = v;
    }
    repaint();
    onCurveEdited();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void LfoShaperEditor::paintBackground(juce::Graphics& g)
{
    const float w = (float)getWidth();
    const float h = (float)getHeight() - 24.0f;

    g.setColour(juce::Colour(0xff1a1a1f));
    g.fillRect(0.0f, 0.0f, w, h);

    g.setColour(juce::Colour(0xff333344));
    g.drawHorizontalLine((int)(h * 0.5f), 0.0f, w);

    g.setColour(juce::Colour(0xff222233));
    g.drawVerticalLine((int)(w * 0.25f), 0.0f, h);
    g.drawVerticalLine((int)(w * 0.5f),  0.0f, h);
    g.drawVerticalLine((int)(w * 0.75f), 0.0f, h);

    g.setColour(juce::Colour(0xff444455));
    g.drawRect(0.0f, 0.0f, w, h, 1.0f);

    g.setColour(juce::Colour(0xff555566));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("LFO SHAPER", 4, 4, (int)w - 8, 14, juce::Justification::left);
}

void LfoShaperEditor::paintOverlay(juce::Graphics& g)
{
    const auto da = getDrawArea();
    const float x = da.getX() + displayPhase * da.getWidth();

    g.setColour(juce::Colour(0xffff8800).withAlpha(0.8f));
    g.drawVerticalLine((int)x, da.getY(), da.getBottom());
}

// ---------------------------------------------------------------------------
// Debounce
// ---------------------------------------------------------------------------

void LfoShaperEditor::onCurveEdited()
{
    startTimer(100);
}

void LfoShaperEditor::timerCallback()
{
    stopTimer();
    notifyListeners();
}
