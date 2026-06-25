cmake_minimum_required(VERSION 3.22)
project(PlatonV1 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Get JUCE automatically (or reuse a local copy via -DFETCHCONTENT_SOURCE_DIR_JUCE=...) ---
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- Plugin formats ---
set(PLATON_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLATON_FORMATS AU)
endif()

juce_add_plugin(PlatonV1
    COMPANY_NAME          "YourLabel"
    PLUGIN_MANUFACTURER_CODE  Ylbl
    PLUGIN_CODE           Pl01
    FORMATS               ${PLATON_FORMATS}
    PRODUCT_NAME          "Platon V1"
    IS_SYNTH              TRUE
    NEEDS_MIDI_INPUT      TRUE
    NEEDS_MIDI_OUTPUT     FALSE
    IS_MIDI_EFFECT        FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(PlatonV1 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(PlatonV1 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

juce_generate_juce_header(PlatonV1)

target_link_libraries(PlatonV1 PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
