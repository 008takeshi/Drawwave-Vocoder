#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <set>
#include "gui/WaveformEditor.h"
#include "gui/BandGainEditor.h"
#include "gui/LfoShaperEditor.h"
#include "gui/KnobPanel.h"
#include "gui/PresetBrowser.h"
#include "gui/PianoKeyboard.h"

class DrawwaveVocoderProcessor;

class DrawwaveVocoderEditor : public juce::AudioProcessorEditor,
                               public juce::Timer,
                               public DrawableCurveComponent::Listener
{
public:
    explicit DrawwaveVocoderEditor(DrawwaveVocoderProcessor&);
    ~DrawwaveVocoderEditor() override;

    void paint              (juce::Graphics&) override;
    void paintOverChildren  (juce::Graphics&) override;
    void resized            () override;
    void timerCallback      () override;
    void visibilityChanged  () override;

    // DrawableCurveComponent::Listener
    void curveChanged(DrawableCurveComponent*) override;

    void syncEditorsFromProcessor();

    bool keyPressed     (const juce::KeyPress&) override;
    bool keyStateChanged(bool isKeyDown)        override;

private:
    DrawwaveVocoderProcessor& proc;
    WaveformEditor            waveformEditor;
    BandGainEditor            bandGainEditor;
    LfoShaperEditor           lfoShaperEditor;
    KnobPanel                 knobPanel;
    PresetBrowser             presetBrowser;
    PianoKeyboard             pianoKeyboard;

    std::set<int> keysHeld;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrawwaveVocoderEditor)
};
