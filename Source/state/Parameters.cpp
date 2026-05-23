#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::masterGain, "Master Gain",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::micGain, "Mic Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::wetDry, "Mic Blend",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::carrierMix, "Carrier Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamID::octave, "Octave", -2, 2, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::noiseMix, "Noise Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::ampAttack, "Amp Attack",
        juce::NormalisableRange<float>(0.1f, 2000.0f, 0.1f, 0.3f), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::ampDecay, "Amp Decay",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::ampSustain, "Amp Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::ampRelease, "Amp Release",
        juce::NormalisableRange<float>(1.0f, 5000.0f, 0.1f, 0.3f), 200.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Phase 2: Vocoder parameters
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamID::bandCount, "Band Count",
        juce::StringArray{ "8", "16", "24", "32" }, 1));  // default index 1 = 16

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::vocoderQ, "Vocoder Q",
        juce::NormalisableRange<float>(1.0f, 10.0f, 0.1f), 4.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::envAttack, "Env Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::envRelease, "Env Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.4f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Phase 6A: Output + Unvoiced
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::hpFreq, "HP Freq",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lpFreq, "LP Freq",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.5f), 20000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::noiseSensitivity, "Noise Sensitivity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::formantShift, "Formant Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("semi")));

    // Phase 5: LFO
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lfoRate, "LFO Rate",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 1.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::lfoSync, "LFO Sync", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamID::lfoSyncDiv, "LFO Sync Division",
        juce::StringArray{ "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lfoDepth, "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamID::lfoDest, "LFO Destination",
        juce::StringArray{ "Formant Shift", "Band Gain Tilt", "Carrier Pitch" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::lfoRetrigger, "LFO Retrigger", false));

    return { params.begin(), params.end() };
}
