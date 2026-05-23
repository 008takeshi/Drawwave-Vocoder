#include "PianoKeyboard.h"

PianoKeyboard::PianoKeyboard(juce::MidiKeyboardState& ks,
                             const std::atomic<uint64_t>& bits0,
                             const std::atomic<uint64_t>& bits1,
                             int start, int end)
    : keyboardState(ks), notesBits0(bits0), notesBits1(bits1),
      startNote(start), endNote(end)
{
    setWantsKeyboardFocus(false);
}

bool PianoKeyboard::isWhiteKey(int note)
{
    const int pc = note % 12;
    return pc==0||pc==2||pc==4||pc==5||pc==7||pc==9||pc==11;
}

int PianoKeyboard::whiteKeyCount() const
{
    int count = 0;
    for (int n = startNote; n < endNote; ++n)
        if (isWhiteKey(n)) ++count;
    return count;
}

bool PianoKeyboard::noteActive(int note) const
{
    // Check keyboardState first — updated on UI thread, gives immediate visual feedback
    if (keyboardState.isNoteOnForChannels(0xffff, note))
        return true;
    // Fallback to notesBits written by audio thread (covers external MIDI)
    if (note < 64)
        return (notesBits0.load(std::memory_order_relaxed) >> note) & 1ULL;
    if (note < 128)
        return (notesBits1.load(std::memory_order_relaxed) >> (note - 64)) & 1ULL;
    return false;
}

int PianoKeyboard::noteAtPosition(float x, float y) const
{
    const float w     = (float)getWidth();
    const float h     = (float)getHeight();
    const int   wkCnt = whiteKeyCount();
    if (wkCnt == 0) return -1;

    const float wkW = w / (float)wkCnt;
    const float bkW = wkW * 0.55f;
    const float bkH = h  * 0.62f;

    // Black keys take priority (drawn on top)
    int wIdx = 0;
    for (int n = startNote; n < endNote; ++n)
    {
        if (isWhiteKey(n)) { ++wIdx; continue; }
        const float bx = (float)wIdx * wkW - bkW * 0.5f;
        if (x >= bx && x < bx + bkW && y >= 0.0f && y < bkH)
            return n;
    }

    // White keys
    wIdx = 0;
    for (int n = startNote; n < endNote; ++n)
    {
        if (!isWhiteKey(n)) continue;
        const float wx = (float)wIdx * wkW;
        if (x >= wx && x < wx + wkW)
            return n;
        ++wIdx;
    }

    return -1;
}

float PianoKeyboard::velocityForY(float y) const
{
    // Top of key = soft (0.1), bottom = loud (1.0)
    return juce::jlimit(0.1f, 1.0f, y / (float)getHeight());
}

void PianoKeyboard::mouseDown(const juce::MouseEvent& e)
{
    const int note = noteAtPosition((float)e.x, (float)e.y);
    if (note >= 0)
    {
        keyboardState.noteOn(1, note, velocityForY((float)e.y));
        mouseDownNote = note;
    }
}

void PianoKeyboard::mouseUp(const juce::MouseEvent&)
{
    if (mouseDownNote >= 0)
    {
        keyboardState.noteOff(1, mouseDownNote, 0.0f);
        mouseDownNote = -1;
    }
}

void PianoKeyboard::mouseDrag(const juce::MouseEvent& e)
{
    const int note = noteAtPosition((float)e.x, (float)e.y);
    if (note != mouseDownNote)
    {
        if (mouseDownNote >= 0)
            keyboardState.noteOff(1, mouseDownNote, 0.0f);
        if (note >= 0)
            keyboardState.noteOn(1, note, velocityForY((float)e.y));
        mouseDownNote = note;
    }
}

void PianoKeyboard::paint(juce::Graphics& g)
{
    const float w     = (float)getWidth();
    const float h     = (float)getHeight();
    const int   wkCnt = whiteKeyCount();
    if (wkCnt == 0) return;

    const float wkW = w / (float)wkCnt;
    const float bkW = wkW * 0.55f;
    const float bkH = h  * 0.62f;

    const juce::Colour wkOn  { 0xff44ddff };
    const juce::Colour wkOff { 0xffe8e8f2 };
    const juce::Colour bkOn  { 0xff0099cc };
    const juce::Colour bkOff { 0xff111122 };
    const juce::Colour kBdr  { 0xff333344 };

    g.setColour(juce::Colour(0xff181820));
    g.fillAll();
    g.setColour(kBdr);
    g.drawHorizontalLine(0, 0.0f, w);

    // White keys
    int wIdx = 0;
    for (int n = startNote; n < endNote; ++n)
    {
        if (!isWhiteKey(n)) continue;
        const float x = (float)wIdx * wkW;
        g.setColour(noteActive(n) ? wkOn : wkOff);
        g.fillRect(x + 0.5f, 1.0f, wkW - 1.0f, h - 1.0f);
        g.setColour(kBdr);
        g.drawRect(x + 0.5f, 1.0f, wkW - 1.0f, h - 1.0f, 0.5f);
        ++wIdx;
    }

    // Black keys (drawn on top)
    wIdx = 0;
    for (int n = startNote; n < endNote; ++n)
    {
        if (isWhiteKey(n)) { ++wIdx; continue; }
        const float x = (float)wIdx * wkW - bkW * 0.5f;
        g.setColour(noteActive(n) ? bkOn : bkOff);
        g.fillRect(x, 1.0f, bkW, bkH);
        g.setColour(kBdr);
        g.drawRect(x, 1.0f, bkW, bkH, 0.5f);
    }
}
