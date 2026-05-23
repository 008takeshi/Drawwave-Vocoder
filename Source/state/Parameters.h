#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamID
{
    inline constexpr const char* masterGain    = "masterGain";
    inline constexpr const char* micGain       = "micGain";
    inline constexpr const char* wetDry        = "wetDry";
    inline constexpr const char* carrierMix    = "carrierMix";
    inline constexpr const char* octave        = "octave";
    inline constexpr const char* noiseMix      = "noiseMix";
    inline constexpr const char* ampAttack     = "ampAttack";
    inline constexpr const char* ampDecay      = "ampDecay";
    inline constexpr const char* ampSustain    = "ampSustain";
    inline constexpr const char* ampRelease    = "ampRelease";

    // Phase 2: Vocoder
    inline constexpr const char* bandCount     = "bandCount";
    inline constexpr const char* vocoderQ      = "vocoderQ";
    inline constexpr const char* envAttack     = "envAttack";
    inline constexpr const char* envRelease    = "envRelease";
    inline constexpr const char* formantShift  = "formantShift";

    // Phase 6A: Output + Unvoiced
    inline constexpr const char* hpFreq           = "hpFreq";
    inline constexpr const char* lpFreq           = "lpFreq";
    inline constexpr const char* noiseSensitivity = "noiseSensitivity";

    // Phase 5: LFO
    inline constexpr const char* lfoRate      = "lfoRate";
    inline constexpr const char* lfoSync      = "lfoSync";
    inline constexpr const char* lfoSyncDiv   = "lfoSyncDiv";
    inline constexpr const char* lfoDepth     = "lfoDepth";
    inline constexpr const char* lfoDest      = "lfoDest";
    inline constexpr const char* lfoRetrigger = "lfoRetrigger";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
