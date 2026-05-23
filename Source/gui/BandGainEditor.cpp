#include "BandGainEditor.h"
#include <cmath>

BandGainEditor::BandGainEditor(int initialBands)
    : DrawableCurveComponent(initialBands, 0.0f, 2.0f)
{
    fill(1.0f);   // neutral gain

    setupButton(btnA,    "A");
    setupButton(btnI,    "I");
    setupButton(btnU,    "U");
    setupButton(btnE,    "E");
    setupButton(btnO,    "O");
    setupButton(btnFlat, "Flat");

    btnA.onClick    = [this]{ loadVowelPreset(VowelA); };
    btnI.onClick    = [this]{ loadVowelPreset(VowelI); };
    btnU.onClick    = [this]{ loadVowelPreset(VowelU); };
    btnE.onClick    = [this]{ loadVowelPreset(VowelE); };
    btnO.onClick    = [this]{ loadVowelPreset(VowelO); };
    btnFlat.onClick = [this]{ loadFlat(); };
}

void BandGainEditor::setupButton(juce::TextButton& btn, const juce::String& label)
{
    btn.setButtonText(label);
    btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff222233));
    btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888899));
    btn.setColour(juce::TextButton::textColourOnId,  juce::Colour(0xff1a1a1f));
    addAndMakeVisible(btn);
}

void BandGainEditor::setNumBands(int n)
{
    if (n == numPoints) return;

    // Resample existing curve to new band count via linear interpolation
    const int srcN = numPoints;
    std::vector<float> resampled((size_t)n, 1.0f);
    for (int i = 0; i < n; ++i)
    {
        const float t  = (n > 1) ? (float)i / (float)(n - 1) * (float)(srcN - 1) : 0.0f;
        const int   lo = juce::jlimit(0, srcN - 1, (int)t);
        const int   hi = std::min(lo + 1, srcN - 1);
        resampled[(size_t)i] = buffer[(size_t)lo] + (t - (float)lo)
                               * (buffer[(size_t)hi] - buffer[(size_t)lo]);
    }
    numPoints = n;
    buffer = resampled;
    repaint();
}

void BandGainEditor::resized()
{
    auto area  = getLocalBounds();
    auto strip = area.removeFromBottom(24);
    const int bw = strip.getWidth() / 6;

    btnA.setBounds   (strip.removeFromLeft(bw));
    btnI.setBounds   (strip.removeFromLeft(bw));
    btnU.setBounds   (strip.removeFromLeft(bw));
    btnE.setBounds   (strip.removeFromLeft(bw));
    btnO.setBounds   (strip.removeFromLeft(bw));
    btnFlat.setBounds(strip);
}

// ---------------------------------------------------------------------------
// Painting — discrete bars + VU overlay
// ---------------------------------------------------------------------------

void BandGainEditor::paintBackground(juce::Graphics& g)
{
    const auto  area = getDrawArea();
    g.setColour(juce::Colour(0xff1a1a1f));
    g.fillRect(area);

    // Neutral-gain line at y = 50% (gain = 1.0)
    const float midY = area.getY() + area.getHeight() * 0.5f;
    g.setColour(juce::Colour(0xff333344));
    g.drawHorizontalLine((int)midY, area.getX(), area.getRight());

    // Border
    g.setColour(juce::Colour(0xff444455));
    g.drawRect(area, 1.0f);

    // Label
    g.setColour(juce::Colour(0xff555566));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("BAND GAIN", (int)area.getRight() - 80, (int)area.getY() + 4,
               76, 14, juce::Justification::right);
}

void BandGainEditor::paint(juce::Graphics& g)
{
    paintBackground(g);

    const auto  area  = getDrawArea();
    const int   n     = (int)buffer.size();
    if (n == 0) return;

    const float bw  = area.getWidth()  / (float)n;
    const float h   = area.getHeight();
    const float oy  = area.getY();

    for (int i = 0; i < n; ++i)
    {
        const float gain = juce::jlimit(0.0f, 2.0f, buffer[(size_t)i]);
        // gain=1.0 → bar height = 50% of area; gain=2.0 → 100%; gain=0 → 0
        const float barH = gain / 2.0f * h;
        const float x    = area.getX() + (float)i * bw;

        // Gain bar
        const float hue = 0.55f + 0.20f * (float)i / (float)n;
        g.setColour(juce::Colour::fromHSV(hue, 0.7f, 0.55f, 1.0f));
        g.fillRect(x + 1.0f, oy + h - barH, bw - 2.0f, barH);

        // VU overlay (live envelope level via atomic read)
        if (vocoderCore != nullptr)
        {
            const float lvl = juce::jlimit(0.0f, 1.0f,
                                           vocoderCore->getBandLevel(i) * 4.0f);
            const float vuH = lvl * h;
            if (vuH > 1.0f)
            {
                // Semi-transparent full bar
                g.setColour(juce::Colour(0xff00ff88).withAlpha(0.30f));
                g.fillRect(x + 1.0f, oy + h - vuH, bw - 2.0f, vuH);
                // Peak line at top of bar
                g.setColour(juce::Colour(0xff00ff88).withAlpha(0.85f));
                g.fillRect(x + 1.0f, oy + h - vuH, bw - 2.0f, 2.0f);
            }
        }
    }

    // dB scale labels — drawn after bars
    const float midY = area.getY() + area.getHeight() * 0.5f;
    g.setColour(juce::Colour(0xff888899));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("+6dB", (int)area.getX() + 2, (int)area.getY() + 2,       40, 10,
               juce::Justification::left);
    g.drawText("  0",  (int)area.getX() + 2, (int)midY - 6,              30, 10,
               juce::Justification::left);
    g.drawText("-inf", (int)area.getX() + 2, (int)area.getBottom() - 12, 30, 10,
               juce::Justification::left);

    // Band frequency labels + tick marks
    // Layout (from bottom): 24 buttons | 14 labels | 5 ticks | draw area
    if (n > 0)
    {
        const int   step     = std::max(1, n / 8);
        const float tickTop  = (float)(getHeight() - 43);
        const float lblY     = (float)(getHeight() - 38 + 2);
        const juce::Colour tickCol { 0xff444455 };

        g.setFont(juce::FontOptions(8.0f));

        for (int i = 0; i < n; i += step)
        {
            const float hz  = bandCenterHz(i);
            const float mx  = area.getX() + ((float)i + 0.5f) * bw;

            // Tick mark (dark)
            g.setColour(tickCol);
            g.drawVerticalLine((int)mx, tickTop, tickTop + 5.0f);

            // Label (bright)
            g.setColour(juce::Colour(0xff888899));
            const juce::String txt = (hz >= 1000.0f)
                ? juce::String(juce::roundToInt(hz / 1000.0f)) + "k"
                : juce::String(juce::roundToInt(hz / 10.0f) * 10);
            g.drawText(txt, (int)mx - 12, (int)lblY, 24, 10,
                       juce::Justification::centred);
        }
    }
}

// ---------------------------------------------------------------------------
// Vowel presets
// ---------------------------------------------------------------------------

float BandGainEditor::bandCenterHz(int band) const noexcept
{
    const int n = (int)buffer.size();
    if (n <= 1) return freqLow;
    const float ratio = std::log(freqHigh / freqLow) / (float)(n - 1);
    return freqLow * std::exp(ratio * (float)band);
}

void BandGainEditor::buildVowel(float f1Hz, float f2Hz, float f3Hz)
{
    const int   n     = (int)buffer.size();
    const float sigma = 0.35f;  // bandwidth in log-freq space (~1/3 octave)

    for (int i = 0; i < n; ++i)
    {
        const float fc  = bandCenterHz(i);
        const float lfc = std::log(fc);

        float gain = 0.15f;  // low baseline
        auto addFormant = [&](float fHz, float boost)
        {
            const float d = (lfc - std::log(fHz)) / sigma;
            gain += boost * std::exp(-0.5f * d * d);
        };

        addFormant(f1Hz, 1.2f);
        addFormant(f2Hz, 1.0f);
        if (f3Hz > 0.0f)
            addFormant(f3Hz, 0.6f);

        buffer[(size_t)i] = juce::jlimit(0.0f, 2.0f, gain);
    }

    repaint();
    onCurveEdited();
}

void BandGainEditor::loadVowelPreset(VowelPreset v)
{
    // Formant frequencies (Hz) from average adult speech
    switch (v)
    {
        case VowelA: buildVowel(800.0f,  1200.0f, 2500.0f); break;
        case VowelI: buildVowel(280.0f,  2600.0f, 3200.0f); break;
        case VowelU: buildVowel(300.0f,   700.0f,  2200.0f); break;
        case VowelE: buildVowel(550.0f,  2000.0f, 2800.0f); break;
        case VowelO: buildVowel(450.0f,   700.0f, 2400.0f); break;
    }
}

void BandGainEditor::loadFlat()
{
    fill(1.0f);
    onCurveEdited();
}

// ---------------------------------------------------------------------------
// Debounce
// ---------------------------------------------------------------------------

void BandGainEditor::onCurveEdited()
{
    startTimer(80);
}
