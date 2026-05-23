#include "WaveformEditor.h"
#include "../dsp/MipmapTable.h"
#include <cmath>

WaveformEditor::WaveformEditor()
    : DrawableCurveComponent(MipmapTable::TableSize, -1.0f, 1.0f)
{
    setupButton(btnSine,  "Sine");
    setupButton(btnSaw,   "Saw");
    setupButton(btnSqr,   "Square");
    setupButton(btnTri,   "Tri");
    setupButton(btnPulse, "Pulse");

    btnSine.onClick  = [this]{ loadPreset(Sine); };
    btnSaw.onClick   = [this]{ loadPreset(Saw); };
    btnSqr.onClick   = [this]{ loadPreset(Square); };
    btnTri.onClick   = [this]{ loadPreset(Triangle); };
    btnPulse.onClick = [this]{ loadPreset(Pulse); };

    loadPreset(Saw);
}

void WaveformEditor::setupButton(juce::TextButton& btn, const juce::String& label)
{
    btn.setButtonText(label);
    btn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff222233));
    btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d9ff));
    btn.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xff888899));
    btn.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xff1a1a1f));
    addAndMakeVisible(btn);
}

void WaveformEditor::resized()
{
    auto area  = getLocalBounds();
    auto strip = area.removeFromBottom(24);
    const int bw = strip.getWidth() / 5;
    btnSine.setBounds (strip.removeFromLeft(bw));
    btnSaw.setBounds  (strip.removeFromLeft(bw));
    btnSqr.setBounds  (strip.removeFromLeft(bw));
    btnTri.setBounds  (strip.removeFromLeft(bw));
    btnPulse.setBounds(strip);
}

void WaveformEditor::loadPreset(PresetShape shape, float pulseWidth)
{
    const int N = MipmapTable::TableSize;
    buffer.resize((size_t)N);

    for (int i = 0; i < N; ++i)
    {
        const float phase = (float)i / (float)N;
        float v = 0.0f;
        switch (shape)
        {
            case Sine:     v = std::sin(juce::MathConstants<float>::twoPi * phase); break;
            case Saw:      v = 2.0f * phase - 1.0f; break;
            case Square:   v = (phase < 0.5f) ? 1.0f : -1.0f; break;
            case Triangle: v = 1.0f - 4.0f * std::abs(phase - 0.5f); break;
            case Pulse:    v = (phase < pulseWidth) ? 1.0f : -1.0f; break;
        }
        buffer[(size_t)i] = v;
    }

    repaint();
    onCurveEdited();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void WaveformEditor::paintBackground(juce::Graphics& g)
{
    const float w = (float)getWidth();
    const float h = (float)getHeight() - 24.0f; // leave space for buttons

    g.setColour(juce::Colour(0xff1a1a1f));
    g.fillRect(0.0f, 0.0f, w, h);

    // Zero line
    g.setColour(juce::Colour(0xff333344));
    g.drawHorizontalLine((int)(h * 0.5f), 0.0f, w);

    // Quarter lines
    g.setColour(juce::Colour(0xff222233));
    g.drawHorizontalLine((int)(h * 0.25f), 0.0f, w);
    g.drawHorizontalLine((int)(h * 0.75f), 0.0f, w);
    g.drawVerticalLine  ((int)(w * 0.25f), 0.0f, h);
    g.drawVerticalLine  ((int)(w * 0.5f),  0.0f, h);
    g.drawVerticalLine  ((int)(w * 0.75f), 0.0f, h);

    // Border
    g.setColour(juce::Colour(0xff444455));
    g.drawRect(0.0f, 0.0f, w, h, 1.0f);

    // Label
    g.setColour(juce::Colour(0xff555566));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("CARRIER WAVEFORM", 4, 4, (int)w - 8, 14, juce::Justification::left);
}

// ---------------------------------------------------------------------------
// Debounce
// ---------------------------------------------------------------------------

void WaveformEditor::onCurveEdited()
{
    startTimer(100); // restart 100 ms debounce
}

void WaveformEditor::timerCallback()
{
    stopTimer();
    notifyListeners();
}
