#pragma once
#include "DrawableCurveComponent.h"
#include "../dsp/VocoderCore.h"

// Per-band gain editor. Bar drawing replaces the line curve from the base class.
// VU overlay reads band envelopes from VocoderCore via atomic reads.
// No VU timer — repaints are driven by PluginEditor's 30fps timer.
class BandGainEditor : public DrawableCurveComponent,
                       private juce::Timer
{
public:
    explicit BandGainEditor (int initialBands = 16);
    ~BandGainEditor() override { stopTimer(); }

    void setNumBands(int n);
    void setFreqRange(float low, float high) { freqLow = low; freqHigh = high; }

    // Set pointer for VU reads. Call setVocoderCore(nullptr) in PluginEditor dtor.
    void setVocoderCore(const VocoderCore* core) noexcept { vocoderCore = core; }

    void resized() override;
    void paint   (juce::Graphics&) override;   // custom: bars, not a line

    enum VowelPreset { VowelA, VowelI, VowelU, VowelE, VowelO };
    void loadVowelPreset(VowelPreset v);
    void loadFlat();

protected:
    juce::Rectangle<float> getDrawArea() const noexcept override
    {
        return { 0.0f, 0.0f, (float)getWidth(), (float)getHeight() - 43.0f }; // 24 buttons + 14 freq labels + 5 ticks
    }

    void paintBackground(juce::Graphics&) override;
    void onCurveEdited  () override;

private:
    const VocoderCore* vocoderCore = nullptr;
    juce::TextButton   btnA, btnI, btnU, btnE, btnO, btnFlat;
    float              freqLow  = 80.0f;
    float              freqHigh = 11000.0f;

    float bandCenterHz(int band) const noexcept;
    void  buildVowel  (float f1Hz, float f2Hz, float f3Hz = -1.0f);
    void  setupButton (juce::TextButton&, const juce::String&);
    void  timerCallback() override { stopTimer(); notifyListeners(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandGainEditor)
};
