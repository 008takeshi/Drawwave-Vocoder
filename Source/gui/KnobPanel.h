#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class KnobPanel : public juce::Component
{
public:
    explicit KnobPanel(juce::AudioProcessorValueTreeState& apvts);
    ~KnobPanel() override;

    void resized() override;
    void paint  (juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    static constexpr auto R = juce::Slider::RotaryVerticalDrag;
    static constexpr auto T = juce::Slider::TextBoxBelow;

    // LFO section
    juce::Slider      lfoRateSlider   { R, T };
    juce::Slider      lfoDepthSlider  { R, T };
    juce::ComboBox    lfoDestBox;
    juce::ToggleButton lfoSyncBtn     { "Sync" };
    juce::ComboBox    lfoSyncDivBox;
    juce::ToggleButton lfoRetrigBtn   { "Retrig" };

    // Vocoder section
    juce::Slider      micGainSlider   { R, T };
    juce::Slider      formantSlider   { R, T };
    juce::Slider      envAttackSlider { R, T };
    juce::Slider      envReleaseSlider{ R, T };
    juce::ComboBox    bandCountBox;

    // Output section
    juce::Slider      wetDrySlider    { R, T };
    juce::Slider      carrierMixSlider{ R, T };
    juce::Slider      hpFreqSlider    { R, T };
    juce::Slider      lpFreqSlider    { R, T };
    juce::Slider      masterGainSlider{ R, T };
    juce::Slider      noiseMixSlider  { R, T };

    // APVTS attachments (declared after sliders — destroyed before sliders)
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAtt> lfoRateAtt, lfoDepthAtt;
    std::unique_ptr<ButtonAtt> lfoSyncAtt, lfoRetrigAtt;
    std::unique_ptr<ComboAtt>  lfoDestAtt, lfoSyncDivAtt;
    std::unique_ptr<SliderAtt> micGainAtt;
    std::unique_ptr<SliderAtt> formantAtt, envAttackAtt, envReleaseAtt;
    std::unique_ptr<ComboAtt>  bandCountAtt;
    std::unique_ptr<SliderAtt> wetDryAtt, carrierMixAtt, hpFreqAtt, lpFreqAtt, masterGainAtt, noiseMixAtt;

    const bool standalone;

    void styleSlider(juce::Slider& s);
    void styleToggle(juce::ToggleButton& b);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobPanel)
};
