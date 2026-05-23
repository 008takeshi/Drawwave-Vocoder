#include "PresetManager.h"
#include "../PluginProcessor.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <BinaryData.h>

using json = nlohmann::json;

// ---- Factory preset registry -----------------------------------------------

struct FactoryEntry { const char* resource; const char* displayName; };

static const FactoryEntry kFactory[] = {
    { "Init_dvoc",          "Init"          },
    { "Classic_Robot_dvoc", "Classic Robot" },
    { "Daft_Style_dvoc",    "Daft Style"    },
    { "Whisper_Chorus_dvoc","Whisper Chorus" },
    { "Talking_Pad_dvoc",   "Talking Pad"   },
    { "Buzz_Lead_dvoc",     "Buzz Lead"     },
    { "Vintage_70s_dvoc",   "Vintage 70s"   },
    { "Choir_dvoc",         "Choir"         },
    { "Spectral_Wash_dvoc", "Spectral Wash" },
    { "Glitch_dvoc",        "Glitch"        },
    { "Bass_Vocoder_dvoc",  "Bass Vocoder"  },
    { "Sci_Fi_Voice_dvoc",  "Sci-Fi Voice"  },
    { "Underwater_dvoc",    "Polar Glow"    },
    { "Megaphone_dvoc",     "Megaphone"     },
    { "Crystal_dvoc",       "Crystal"       },
    { "Monster_dvoc",       "Monster"       },
};

// All APVTS parameter IDs in declaration order
static const char* const kParamIDs[] = {
    "masterGain", "wetDry", "carrierMix", "octave", "noiseMix",
    "ampAttack", "ampDecay", "ampSustain", "ampRelease",
    "micGain", "bandCount", "vocoderQ", "envAttack", "envRelease",
    "formantShift", "hpFreq", "lpFreq", "noiseSensitivity",
    "lfoRate", "lfoSync", "lfoSyncDiv", "lfoDepth", "lfoDest", "lfoRetrigger"
};

// ---- Constructor -------------------------------------------------------------

PresetManager::PresetManager(DrawwaveVocoderProcessor& p) : proc(p)
{
    buildPresetList();
}

void PresetManager::buildPresetList()
{
    presets.clear();
    for (auto& e : kFactory)
    {
        PresetInfo info;
        info.name      = e.displayName;
        info.isFactory = true;
        presets.push_back(std::move(info));
    }
    refreshUserPresets();
}

void PresetManager::refreshUserPresets()
{
    presets.erase(
        std::remove_if(presets.begin(), presets.end(),
                       [](const PresetInfo& p) { return !p.isFactory; }),
        presets.end());

    auto dir = getUserPresetsDir();
    if (!dir.exists()) dir.createDirectory();

    for (auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.dvoc"))
    {
        PresetInfo info;
        info.name      = f.getFileNameWithoutExtension();
        info.file      = f;
        info.isFactory = false;
        presets.push_back(std::move(info));
    }
}

// ---- Load -------------------------------------------------------------------

bool PresetManager::loadPreset(const PresetInfo& info)
{
    if (info.isFactory)
    {
        for (auto& e : kFactory)
        {
            if (juce::String(e.displayName) != info.name) continue;
            int size = 0;
            const char* data = BinaryData::getNamedResource(e.resource, size);
            if (!data || size == 0) return false;
            if (!applyJsonString(juce::String::fromUTF8(data, size))) return false;
            currentName = info.name;
            return true;
        }
        return false;
    }

    if (!info.file.existsAsFile()) return false;
    if (!applyJsonString(info.file.loadFileAsString())) return false;
    currentName = info.name;
    return true;
}

// ---- Save -------------------------------------------------------------------

bool PresetManager::savePreset(const juce::String& name)
{
    auto dir = getUserPresetsDir();
    if (!dir.exists()) dir.createDirectory();

    const juce::String safe = name.replaceCharacters("/\\:*?\"<>|", "_________");
    const juce::String text = serializeToJson(name);
    if (!dir.getChildFile(safe + ".dvoc").replaceWithText(text)) return false;

    currentName = name;
    refreshUserPresets();
    return true;
}

bool PresetManager::deleteUserPreset(const PresetInfo& info)
{
    if (info.isFactory || !info.file.existsAsFile()) return false;
    if (!info.file.deleteFile()) return false;
    refreshUserPresets();
    return true;
}

// ---- A/B comparison ---------------------------------------------------------

void PresetManager::captureA() { proc.getStateInformation(slotA); }
void PresetManager::captureB() { proc.getStateInformation(slotB); }

bool PresetManager::recallA()
{
    if (slotA.getSize() == 0) return false;
    proc.setStateInformation(slotA.getData(), (int)slotA.getSize());
    return true;
}

bool PresetManager::recallB()
{
    if (slotB.getSize() == 0) return false;
    proc.setStateInformation(slotB.getData(), (int)slotB.getSize());
    return true;
}

// ---- User presets directory -------------------------------------------------

juce::File PresetManager::getUserPresetsDir()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("TIM And Friends/DrawwaveVocoder/Presets");
}

// ---- Wave generation --------------------------------------------------------

std::vector<float> PresetManager::generateCarrierWave(const std::string& type, int size)
{
    std::vector<float> v(size);
    constexpr float twoPi = 6.2831853f;
    for (int i = 0; i < size; ++i)
    {
        const float t = (float)i / size;
        if      (type == "sine")     v[i] = std::sin(twoPi * t);
        else if (type == "square")   v[i] = t < 0.5f ? 1.0f : -1.0f;
        else if (type == "triangle") v[i] = t < 0.5f ? 4.0f * t - 1.0f : 3.0f - 4.0f * t;
        else if (type == "pulse")    v[i] = t < 0.25f ? 1.0f : -1.0f;
        else                         v[i] = 2.0f * t - 1.0f;  // default: saw
    }
    return v;
}

std::vector<float> PresetManager::generateLfoWave(const std::string& type, int size)
{
    std::vector<float> v(size);
    constexpr float twoPi = 6.2831853f;
    for (int i = 0; i < size; ++i)
    {
        const float t = (float)i / size;
        if      (type == "triangle") v[i] = t < 0.5f ? 4.0f * t - 1.0f : 3.0f - 4.0f * t;
        else if (type == "ramp")     v[i] = 2.0f * t - 1.0f;
        else                         v[i] = std::sin(twoPi * t);  // default: sine
    }
    return v;
}

// ---- JSON serialization / deserialization -----------------------------------

juce::String PresetManager::serializeToJson(const juce::String& name) const
{
    json j;
    j["version"] = 1;
    j["name"]    = name.toStdString();

    json params;
    for (auto* id : kParamIDs)
        if (auto* raw = proc.apvts.getRawParameterValue(id))
            params[id] = raw->load();
    j["params"] = params;

    j["carrierWave"] = "custom";
    {
        const auto& buf = proc.getWaveform();
        j["carrierWaveData"] = std::vector<float>(buf.begin(), buf.end());
    }

    j["lfoWave"] = "custom";
    {
        const auto& buf = proc.getLfoWaveform();
        j["lfoWaveData"] = std::vector<float>(buf.begin(), buf.end());
    }

    {
        const int n = proc.getVocoderCore().getNumBands();
        std::vector<float> gains(n);
        for (int i = 0; i < n; ++i)
            gains[i] = proc.getVocoderCore().getBandGain(i);
        j["bandGains"] = gains;
    }

    return juce::String(j.dump(2));
}

bool PresetManager::applyJsonString(const juce::String& jsonStr)
{
    json j;
    try { j = json::parse(jsonStr.toStdString()); }
    catch (...) { return false; }

    // Parameters
    if (j.contains("params") && j["params"].is_object())
    {
        for (auto& [id, val] : j["params"].items())
        {
            if (!val.is_number()) continue;
            auto* param = dynamic_cast<juce::RangedAudioParameter*>(
                              proc.apvts.getParameter(id));
            if (!param) continue;
            const float v = val.get<float>();
            param->setValueNotifyingHost(param->convertTo0to1(v));
        }
    }

    // Carrier waveform
    const std::string waveType = j.value("carrierWave", "saw");
    if (waveType == "custom" && j.contains("carrierWaveData") && j["carrierWaveData"].is_array())
    {
        auto data = j["carrierWaveData"].get<std::vector<float>>();
        if (!data.empty()) proc.setWaveform(data);
    }
    else
    {
        proc.setWaveform(generateCarrierWave(waveType));
    }

    // LFO waveform
    const std::string lfoType = j.value("lfoWave", "sine");
    if (lfoType == "custom" && j.contains("lfoWaveData") && j["lfoWaveData"].is_array())
    {
        auto data = j["lfoWaveData"].get<std::vector<float>>();
        if (!data.empty()) proc.setLfoWaveform(data);
    }
    else
    {
        proc.setLfoWaveform(generateLfoWave(lfoType));
    }

    // Band gains
    if (j.contains("bandGains") && j["bandGains"].is_array())
    {
        auto gains = j["bandGains"].get<std::vector<float>>();
        proc.setBandGains(gains);
    }

    return true;
}
