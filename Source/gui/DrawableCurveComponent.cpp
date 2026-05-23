#include "DrawableCurveComponent.h"

DrawableCurveComponent::DrawableCurveComponent(int nPoints, float minVal, float maxVal)
    : minValue(minVal), maxValue(maxVal), numPoints(nPoints)
{
    buffer.assign((size_t)numPoints, 0.0f);
    setOpaque(false);
}

void DrawableCurveComponent::setBuffer(const std::vector<float>& b)
{
    buffer = b;
    buffer.resize((size_t)numPoints, 0.0f);
    repaint();
}

void DrawableCurveComponent::smooth(int iterations)
{
    const int n = (int)buffer.size();
    std::vector<float> tmp((size_t)n);
    for (int iter = 0; iter < iterations; ++iter)
    {
        for (int i = 0; i < n; ++i)
            tmp[(size_t)i] = (buffer[(size_t)((i - 2 + n) % n)]
                            + buffer[(size_t)((i - 1 + n) % n)]
                            + buffer[(size_t)i]
                            + buffer[(size_t)((i + 1) % n)]
                            + buffer[(size_t)((i + 2) % n)]) / 5.0f;
        buffer = tmp;
    }
    repaint();
    onCurveEdited();
}

void DrawableCurveComponent::invert()
{
    for (auto& v : buffer) v = -v;
    repaint();
    onCurveEdited();
}

void DrawableCurveComponent::normalize()
{
    float peak = 0.0f;
    for (auto v : buffer) peak = std::max(peak, std::abs(v));
    if (peak > 1e-6f)
        for (auto& v : buffer) v /= peak;
    repaint();
    onCurveEdited();
}

void DrawableCurveComponent::fill(float value)
{
    std::fill(buffer.begin(), buffer.end(), value);
    repaint();
    onCurveEdited();
}

// ---------------------------------------------------------------------------
// Mouse handling
// ---------------------------------------------------------------------------

void DrawableCurveComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Smooth");
        menu.addItem(2, "Invert");
        menu.addItem(3, "Normalize");
        menu.addSeparator();
        menu.addItem(4, "Clear");

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetScreenArea({e.getScreenX(), e.getScreenY(), 1, 1}),
            [this](int result)
            {
                switch (result)
                {
                    case 1: smooth(4);   break;
                    case 2: invert();    break;
                    case 3: normalize(); break;
                    case 4: fill(0.0f); break;
                    default: break;
                }
            });
        return;
    }

    const auto  da = getDrawArea();
    const float nx = ((float)e.x - da.getX()) / da.getWidth();
    const float ny = ((float)e.y - da.getY()) / da.getHeight();
    lastMousePos = { nx, ny };
    applyMousePos(nx, ny);
}

void DrawableCurveComponent::mouseDrag(const juce::MouseEvent& e)
{
    const auto  da = getDrawArea();
    const float nx = ((float)e.x - da.getX()) / da.getWidth();
    const float ny = ((float)e.y - da.getY()) / da.getHeight();
    paintLine(lastMousePos.x, lastMousePos.y, nx, ny);
    lastMousePos = { nx, ny };
}

void DrawableCurveComponent::mouseUp(const juce::MouseEvent&)
{
    lastMousePos = { -1.0f, -1.0f };
    onCurveEdited();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void DrawableCurveComponent::paint(juce::Graphics& g)
{
    paintBackground(g);

    const int   n     = (int)buffer.size();
    const auto  da    = getDrawArea();
    const float w     = da.getWidth();
    const float h     = da.getHeight();
    const float ox    = da.getX();
    const float oy    = da.getY();
    const float range = maxValue - minValue;

    // Draw curve
    juce::Path path;
    for (int i = 0; i < n; ++i)
    {
        const float x = ox + (float)i / (float)n * w;
        const float v = juce::jlimit(minValue, maxValue, buffer[(size_t)i]);
        const float y = oy + h - (v - minValue) / range * h;

        if (i == 0) path.startNewSubPath(x, y);
        else        path.lineTo(x, y);
    }
    // Close visually
    {
        const float v = juce::jlimit(minValue, maxValue, buffer[0]);
        path.lineTo(ox + w, oy + h - (v - minValue) / range * h);
    }

    g.setColour(juce::Colour(0xff00d9ff));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    paintOverlay(g);
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void DrawableCurveComponent::applyMousePos(float normX, float normY)
{
    const int n = (int)buffer.size();
    const int idx = juce::jlimit(0, n - 1, (int)(normX * (float)n));
    const float val = maxValue - normY * (maxValue - minValue);
    buffer[(size_t)idx] = juce::jlimit(minValue, maxValue, val);
    repaint();
}

void DrawableCurveComponent::paintLine(float x0, float y0, float x1, float y1)
{
    const int n = (int)buffer.size();
    const int i0 = juce::jlimit(0, n - 1, (int)(x0 * (float)n));
    const int i1 = juce::jlimit(0, n - 1, (int)(x1 * (float)n));

    const int lo = std::min(i0, i1);
    const int hi = std::max(i0, i1);

    for (int i = lo; i <= hi; ++i)
    {
        const float t = (hi > lo) ? (float)(i - lo) / (float)(hi - lo) : 0.0f;
        const float normY = y0 + t * (y1 - y0);
        const float val   = maxValue - normY * (maxValue - minValue);
        buffer[(size_t)i] = juce::jlimit(minValue, maxValue, val);
    }
    repaint();
}

void DrawableCurveComponent::onCurveEdited()
{
    notifyListeners();
}

void DrawableCurveComponent::notifyListeners()
{
    listeners.call([this](Listener& l){ l.curveChanged(this); });
}
