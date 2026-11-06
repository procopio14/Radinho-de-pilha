cmake_minimum_required(VERSION 3.22)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()
if(MSVC)
  # runtime estatico: o plugin roda sem precisar instalar VC++ Redistributable
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(Radinho VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Radinho
  COMPANY_NAME "Radinho Audio"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Rdnh
  PLUGIN_CODE Rdp1
  FORMATS VST3 Standalone
  VST3_CATEGORIES Fx
  PRODUCT_NAME "RDP")

juce_generate_juce_header(Radinho)

target_sources(Radinho PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h)

target_compile_definitions(Radinho PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Radinho
  PRIVATE
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
