#pragma once
#include "DrawableCurveComponent.h"
#include "../dsp/DrawableLfo.h"

// LFO shape editor: 512-point curve with a phase-playback indicator line.
// Inherits DrawableCurveComponent for mouse-drawable curve;
// inherits juce::Timer for 100 ms edit debounce.
class LfoShaperEditor : public DrawableCurveComponent,
                        private juce::Timer
{
public:
    LfoShaperEditor();
    ~LfoShaperEditor() override { stopTimer(); }

    void resized() override;

    // Called by PluginEditor at 30 fps to move the playhead indicator.
    void setPhase(float normalizedPhase) noexcept { displayPhase = normalizedPhase; }

    enum PresetShape { Sine, Triangle, Ramp, Random };
    void loadPreset(PresetShape shape);

protected:
    juce::Rectangle<float> getDrawArea() const noexcept override
        { return { 0.0f, 0.0f, (float)getWidth(), (float)getHeight() - 24.0f }; }

    void paintBackground(juce::Graphics&) override;
    void paintOverlay   (juce::Graphics&) override;
    void onCurveEdited  () override;

private:
    float displayPhase = 0.0f;

    juce::TextButton btnSine, btnTri, btnRamp, btnRand;

    void timerCallback() override;
    void setupButton(juce::TextButton&, const juce::String& label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoShaperEditor)
};
