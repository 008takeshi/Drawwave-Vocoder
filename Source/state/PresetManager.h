#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>

class DrawwaveVocoderProcessor;

class PresetManager
{
public:
    static constexpr int WaveformSize = 2048;
    static constexpr int LfoSize      = 512;

    struct PresetInfo
    {
        juce::String name;
        juce::File   file;
        bool         isFactory { false };
    };

    explicit PresetManager(DrawwaveVocoderProcessor& proc);

    const std::vector<PresetInfo>& getPresets() const noexcept { return presets; }
    void refreshUserPresets();

    bool loadPreset      (const PresetInfo& info);
    bool savePreset      (const juce::String& name);
    bool deleteUserPreset(const PresetInfo& info);

    void captureA();
    void captureB();
    bool recallA();
    bool recallB();
    bool hasSlotA() const noexcept { return slotA.getSize() > 0; }
    bool hasSlotB() const noexcept { return slotB.getSize() > 0; }

    const juce::String& getCurrentName() const noexcept { return currentName; }

    static juce::File getUserPresetsDir();

    static std::vector<float> generateCarrierWave(const std::string& type, int size = WaveformSize);
    static std::vector<float> generateLfoWave    (const std::string& type, int size = LfoSize);

private:
    DrawwaveVocoderProcessor& proc;
    std::vector<PresetInfo>   presets;
    juce::String              currentName { "Init" };
    juce::MemoryBlock         slotA, slotB;

    void buildPresetList();
    bool applyJsonString(const juce::String& jsonStr);
    juce::String serializeToJson(const juce::String& name) const;
};
