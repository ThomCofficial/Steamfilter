cmake_minimum_required(VERSION 3.22)
project(SteamFilter VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
add_library(SteamDSP STATIC Source/DSP/SteamDSP.cpp)
target_include_directories(SteamDSP PUBLIC Source/DSP)
if(MSVC)
  target_compile_options(SteamDSP PRIVATE /W4)
else()
  target_compile_options(SteamDSP PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
  add_executable(SteamDSPStress Tests/StressTests.cpp)
  target_link_libraries(SteamDSPStress PRIVATE SteamDSP)
  add_test(NAME SteamDSPStress COMMAND SteamDSPStress)
  add_executable(SteamDSPModules Tests/ModuleTests.cpp)
  target_link_libraries(SteamDSPModules PRIVATE SteamDSP)
  add_test(NAME SteamDSPModules COMMAND SteamDSPModules)
endif()
option(STEAMFILTER_BUILD_PLUGIN "Build JUCE VST3 (requires JUCE source or network)" OFF)
if(STEAMFILTER_BUILD_PLUGIN)
  include(FetchContent)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
  else()
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()
  juce_add_plugin(SteamFilter
    COMPANY_NAME "ThomC"
    BUNDLE_ID "com.thomc.steamfilter"
    PLUGIN_MANUFACTURER_CODE ThmC
    PLUGIN_CODE StFl
    FORMATS VST3
    PRODUCT_NAME "SteamFilter"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  juce_add_binary_data(SteamFilterArt SOURCES Assets/ApprovedFaceplate.jpg Assets/KnobMain.png Assets/KnobMini.png Assets/GaugeFace.png)
  target_sources(SteamFilter PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_link_libraries(SteamFilter PRIVATE SteamDSP SteamFilterArt juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)
  target_compile_definitions(SteamFilter PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  juce_generate_juce_header(SteamFilter)
endif()