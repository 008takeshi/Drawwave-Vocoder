#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../state/PresetManager.h"

class DrawwaveVocoderProcessor;

class PresetBrowser : public juce::Component
{
public:
    explicit PresetBrowser(DrawwaveVocoderProcessor& proc);
    ~PresetBrowser() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

    void refreshList();

    // Called after any preset/A-B state load. Set by PluginEditor to sync editors.
    std::function<void()> onPresetLoaded;

private:
    DrawwaveVocoderProcessor& proc;
    PresetManager             presetManager;

    // ---- ListBox model ----
    struct Model : juce::ListBoxModel
    {
        PresetBrowser* owner = nullptr;

        int  getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool sel) override;
        void listBoxItemClicked       (int row, const juce::MouseEvent&) override;
        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    };

    Model         model;
    juce::ListBox listBox;

    juce::TextButton btnSave, btnDelete, btnCapA, btnRecA, btnCapB, btnRecB;

    // selected preset index (-1 = none)
    int selectedRow = -1;

    void onSave();
    void onDelete();
    void onCapA();
    void onRecA();
    void onCapB();
    void onRecB();
    void loadRow(int row);
    void updateButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
