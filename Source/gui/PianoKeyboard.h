#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>

class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard(juce::MidiKeyboardState& ks,
                  const std::atomic<uint64_t>& bits0,
                  const std::atomic<uint64_t>& bits1,
                  int startNote = 36, int endNote = 84);

    void paint     (juce::Graphics&)         override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    juce::MidiKeyboardState&     keyboardState;
    const std::atomic<uint64_t>& notesBits0;
    const std::atomic<uint64_t>& notesBits1;
    int startNote, endNote;
    int mouseDownNote = -1;

    static bool isWhiteKey   (int note);
    int         whiteKeyCount() const;
    int         noteAtPosition(float x, float y) const;
    bool        noteActive    (int note) const;
    float       velocityForY  (float y) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboard)
};
