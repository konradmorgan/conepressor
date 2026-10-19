cmake_minimum_required(VERSION 3.22)
project(ConePressor VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

option(COPY_AFTER_BUILD "Plugin nach dem Build in den Plugin-Ordner kopieren" ON)

juce_add_plugin(ConePressor
    COMPANY_NAME "Konbeatz"
    PLUGIN_MANUFACTURER_CODE Kbtz
    PLUGIN_CODE Cnpr
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "ConePressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD}
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect)

# Katzenbild wird als BinaryData ins Plugin einkompiliert
juce_add_binary_data(CatAssets SOURCES Assets/cat.png)

target_sources(ConePressor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ConePressor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ConePressor
    PRIVATE
        CatAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
