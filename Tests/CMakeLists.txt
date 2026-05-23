CPMAddPackage("gh:google/googletest@1.14.0")

add_executable(VocoderTests
    dsp/SmokeTest.cpp
)

target_link_libraries(VocoderTests
    PRIVATE
        GTest::gtest_main
        juce::juce_audio_utils
        juce::juce_dsp
)

target_include_directories(VocoderTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_compile_definitions(VocoderTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

include(GoogleTest)
gtest_discover_tests(VocoderTests)
