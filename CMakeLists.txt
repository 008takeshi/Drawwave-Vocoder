cmake_minimum_required(VERSION 3.22)
project(DrawwaveVocoder VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "")

include(cmake/CPM.cmake)

CPMAddPackage("gh:juce-framework/JUCE#8.0.4")
CPMAddPackage("gh:nlohmann/json@3.11.3")

if(BUILD_TESTING)
    CPMAddPackage("gh:google/googletest@1.14.0")
endif()

juce_add_binary_data(DrawwaveVocoderPresets
    HEADER_NAME BinaryData.h
    NAMESPACE   BinaryData
    SOURCES
        Presets/Init.dvoc
        Presets/Classic_Robot.dvoc
        Presets/Daft_Style.dvoc
        Presets/Whisper_Chorus.dvoc
        Presets/Talking_Pad.dvoc
        Presets/Buzz_Lead.dvoc
        Presets/Vintage_70s.dvoc
        Presets/Choir.dvoc
        Presets/Spectral_Wash.dvoc
        Presets/Glitch.dvoc
        Presets/Bass_Vocoder.dvoc
        Presets/Sci_Fi_Voice.dvoc
        Presets/Underwater.dvoc
        Presets/Megaphone.dvoc
        Presets/Crystal.dvoc
        Presets/Monster.dvoc
)

if(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
    set(PLUGIN_COPY_AFTER_BUILD TRUE)
    set(APPLE_PLUGIN_ARGS
        AU_MAIN_TYPE                  "kAudioUnitType_MusicDevice"
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT    "Drawwave Vocoder needs microphone access for the modulator input."
    )
else()
    set(PLUGIN_FORMATS VST3 Standalone)
    set(PLUGIN_COPY_AFTER_BUILD FALSE)
    set(APPLE_PLUGIN_ARGS "")
endif()

juce_add_plugin(DrawwaveVocoder
    COMPANY_NAME              "TIM And Friends"
    BUNDLE_ID                 "com.timandfriends.DrawwaveVocoder"
    PLUGIN_MANUFACTURER_CODE  TmAF
    PLUGIN_CODE               Dvoc
    FORMATS                   ${PLUGIN_FORMATS}
    PRODUCT_NAME              "Drawwave Vocoder"
    ICON_BIG                  "${CMAKE_CURRENT_SOURCE_DIR}/DRWWVVC.png"
    ICON_SMALL                "${CMAKE_CURRENT_SOURCE_DIR}/DRWWVVC.png"
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS       TRUE
    COPY_PLUGIN_AFTER_BUILD           ${PLUGIN_COPY_AFTER_BUILD}
    VST3_CATEGORIES           "Instrument" "Synth"
    ${APPLE_PLUGIN_ARGS}
)

target_sources(DrawwaveVocoder
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/state/Parameters.cpp
        Source/dsp/VoiceManager.cpp
        Source/dsp/FilterBank.cpp
        Source/dsp/EnvelopeFollower.cpp
        Source/dsp/VocoderCore.cpp
        Source/dsp/MipmapTable.cpp
        Source/dsp/WavetableOscillator.cpp
        Source/gui/DrawableCurveComponent.cpp
        Source/gui/WaveformEditor.cpp
        Source/gui/BandGainEditor.cpp
        Source/gui/LfoShaperEditor.cpp
        Source/gui/KnobPanel.cpp
        Source/dsp/DrawableLfo.cpp
        Source/dsp/OutputStage.cpp
        Source/dsp/UnvoicedDetector.cpp
        Source/state/PresetManager.cpp
        Source/gui/PresetBrowser.cpp
        Source/gui/PianoKeyboard.cpp
        Source/Standalone/Main.cpp
)

target_include_directories(DrawwaveVocoder PRIVATE Source)

target_compile_definitions(DrawwaveVocoder
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

target_link_libraries(DrawwaveVocoder
    PRIVATE
        DrawwaveVocoderPresets
        juce::juce_audio_utils
        juce::juce_dsp
        nlohmann_json::nlohmann_json
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(APPLE)
    execute_process(
        COMMAND bash -c "security find-identity -v -p codesigning 2>/dev/null | grep -E '(Apple Development|Developer ID Application)' | head -1 | awk '{print $2}'"
        OUTPUT_VARIABLE CODESIGN_IDENTITY
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT CODESIGN_IDENTITY OR CODESIGN_IDENTITY STREQUAL "")
        set(CODESIGN_IDENTITY "-")
    endif()
    message(STATUS "Standalone codesign identity: ${CODESIGN_IDENTITY}")

    add_custom_command(TARGET DrawwaveVocoder_Standalone POST_BUILD
        COMMAND codesign --force --deep --sign "${CODESIGN_IDENTITY}"
            --entitlements "${CMAKE_SOURCE_DIR}/Standalone.entitlements"
            "$<TARGET_BUNDLE_DIR:DrawwaveVocoder_Standalone>"
        COMMENT "Re-signing Standalone.app with microphone entitlement"
        VERBATIM)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(Tests)
endif()
