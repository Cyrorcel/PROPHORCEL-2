cmake_minimum_required(VERSION 3.22)
project(Prophorcel VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()

# JUCE : par défaut téléchargé depuis GitHub. Pour utiliser une copie locale :
#   cmake -B build -DPROPH_JUCE_DIR=/chemin/vers/JUCE
set(PROPH_JUCE_DIR "" CACHE PATH "Dossier JUCE local (optionnel)")
if(PROPH_JUCE_DIR)
    add_subdirectory(${PROPH_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.17-security
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Prophorcel
    COMPANY_NAME "Prophorcel"
    PLUGIN_MANUFACTURER_CODE Prcl
    PLUGIN_CODE Ph01
    FORMATS VST3 Standalone
    PRODUCT_NAME "Prophorcel"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth)

target_sources(Prophorcel PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Prophorcel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Prophorcel
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# Test du moteur sans interface (facultatif) :  cmake -B build -DPROPH_BUILD_TEST=ON
option(PROPH_BUILD_TEST "Compiler le test du moteur" OFF)
if(PROPH_BUILD_TEST)
    add_executable(EngineTest Tests/EngineTest.cpp)
    target_include_directories(EngineTest PRIVATE Source)
    target_link_libraries(EngineTest PRIVATE juce::juce_audio_basics juce::juce_recommended_config_flags)
endif()
