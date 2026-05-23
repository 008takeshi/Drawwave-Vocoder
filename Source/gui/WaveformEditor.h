#pragma once
#include "DrawableCurveComponent.h"
#include "../dsp/MipmapTable.h"

// Waveform editor component: draws a 2048-point periodic waveform.
// Preset buttons populate the buffer with standard waveform shapes.
// Edits are debounced 100 ms before notifying listeners.
class WaveformEditor : public DrawableCurveComponent,
                       private juce::Timer
{
public:
    WaveformEditor();
    ~WaveformEditor() override { stopTimer(); }

    void resized() override;

    enum PresetShape { Sine, Saw, Square, Triangle, Pulse };
    void loadPreset (PresetShape shape, float pulseWidth = 0.25f);

protected:
    juce::Rectangle<float> getDrawArea() const noexcept override
        { return { 0.0f, 0.0f, (float)getWidth(), (float)getHeight() - 24.0f }; }

    void paintBackground (juce::Graphics&) override;
    void onCurveEdited   () override;

private:
    juce::TextButton btnSine, btnSaw, btnSqr, btnTri, btnPulse;

    void timerCallback() override;
    void setupButton   (juce::TextButton&, const juce::String& label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformEditor)
};
