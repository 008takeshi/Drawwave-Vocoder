#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// Base class for mouse-drawable curve components.
// Stores a float buffer that the user edits by clicking and dragging.
// Subclasses override paintBackground() to add gridlines, labels, etc.
class DrawableCurveComponent : public juce::Component
{
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void curveChanged (DrawableCurveComponent*) = 0;
    };

    DrawableCurveComponent (int numPoints, float minVal = -1.0f, float maxVal = 1.0f);

    int getNumPoints() const noexcept { return numPoints; }

    void addListener    (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    const std::vector<float>& getBuffer() const noexcept { return buffer; }
    void setBuffer (const std::vector<float>& b);

    void smooth   (int iterations = 2);
    void invert();
    void normalize();
    void fill     (float value);

    // Mouse drawing
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

    virtual void paint (juce::Graphics&) override;

protected:
    std::vector<float> buffer;
    float minValue, maxValue;
    int   numPoints;

    // Subclasses override to restrict drawing/mouse area (e.g., exclude button strip).
    virtual juce::Rectangle<float> getDrawArea() const noexcept
        { return getLocalBounds().toFloat(); }

    virtual void paintBackground (juce::Graphics&) {}
    virtual void paintOverlay    (juce::Graphics&) {}
    virtual void onCurveEdited();   // called after each edit; subclass can debounce

    void notifyListeners();

private:
    juce::ListenerList<Listener> listeners;
    juce::Point<float> lastMousePos { -1.0f, -1.0f };

    void applyMousePos  (float normX, float normY);
    void paintLine      (float x0, float y0, float x1, float y1);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawableCurveComponent)
};
