cmake_minimum_required(VERSION 3.22)

project(CrazyVisualEQ VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statische MSVC-Runtime -> keine VC++-Redistributable beim Anwender noetig
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CrazyVisualEQ
    COMPANY_NAME            "CrazyAudio"
    PLUGIN_MANUFACTURER_CODE Crzy
    PLUGIN_CODE             Ceq1
    FORMATS                 VST3
    PRODUCT_NAME            "Crazy Visual EQ"
    VST3_CATEGORIES         Fx EQ
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(CrazyVisualEQ PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h)

target_compile_definitions(CrazyVisualEQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(CrazyVisualEQ PRIVATE /utf-8)
endif()

target_link_libraries(CrazyVisualEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
