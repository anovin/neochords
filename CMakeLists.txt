cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) when building on a Mac — must be set before project()
if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()

project(NeoChords VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Formats: VST3 everywhere, plus AU on macOS. Add "Standalone" to FORMATS to get a test app.
set(NEO_FORMATS VST3)
if(APPLE)
    list(APPEND NEO_FORMATS AU)
endif()

juce_add_plugin(NeoChords
    COMPANY_NAME "Amir Novin"
    BUNDLE_ID "com.amirnovin.neochords"
    PRODUCT_NAME "NeoChords"
    PLUGIN_MANUFACTURER_CODE Anov
    PLUGIN_CODE NeoC
    FORMATS ${NEO_FORMATS}
    # Loads as an instrument (generator) so FL Studio puts it on a channel and
    # lets you route its MIDI output to any other plugin.
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

target_sources(NeoChords PRIVATE
    Source/ChordEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NeoChords PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(NeoChords PRIVATE /utf-8)
endif()

target_link_libraries(NeoChords
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Engine unit test (no JUCE needed): cmake --build . --target EngineTest && ./EngineTest
add_executable(EngineTest Tests/EngineTest.cpp Source/ChordEngine.cpp)
