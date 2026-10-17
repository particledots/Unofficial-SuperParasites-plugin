cmake_minimum_required(VERSION 3.22)
project(SuperParasitesPlugin VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  # The GitHub workflow passes -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" for a universal build.
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Plugin formats: AU + VST3 (+ Standalone for quick testing).
set(FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND FORMATS AU)
endif()

juce_add_plugin(SuperParasites
  COMPANY_NAME "particledots"
  PLUGIN_MANUFACTURER_CODE Ptdt
  PLUGIN_CODE SPar
  FORMATS ${FORMATS}
  PRODUCT_NAME "Unofficial SuperParasites"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  AU_MAIN_TYPE kAudioUnitType_MusicEffect   # lets Logic route MIDI to it as a "MIDI-controlled effect"
  COPY_PLUGIN_AFTER_BUILD TRUE)

# --- DSP core: vendored SuperParasites/stmlib code + our JUCE-free wrapper, built with plain flags ---
add_library(superparasites_core STATIC
  src/SuperParasitesEngine.cpp
  third_party/supercell/dsp/correlator.cc
  third_party/supercell/dsp/granular_processor.cc
  third_party/supercell/dsp/kammerl_player.cc
  third_party/supercell/dsp/mu_law.cc
  third_party/supercell/dsp/pvoc/frame_transformation.cc
  third_party/supercell/dsp/pvoc/phase_vocoder.cc
  third_party/supercell/dsp/pvoc/spectral_clouds_transformation.cc
  third_party/supercell/dsp/pvoc/stft.cc
  third_party/supercell/resources.cc
  third_party/stmlib/utils/random.cc
  third_party/stmlib/dsp/units.cc
  third_party/stmlib/dsp/atan.cc)
target_include_directories(superparasites_core PUBLIC src third_party)
target_compile_definitions(superparasites_core PRIVATE TEST=1)
set_target_properties(superparasites_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Some upstream headers rely on <cstdio>/<cstdlib>/<cmath> being pulled in transitively.
if(MSVC)
  target_compile_options(superparasites_core PRIVATE /FIcstdio /FIcstdlib /FIcmath /FIcstring)
else()
  target_compile_options(superparasites_core PRIVATE "SHELL:-include cstdio" "SHELL:-include cstdlib" "SHELL:-include cmath" "SHELL:-include cstring" -Wno-unused-local-typedef -Wno-narrowing)
endif()

# --- The plugin ---
target_sources(SuperParasites PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp)
target_compile_definitions(SuperParasites PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SuperParasites PRIVATE
  superparasites_core
  juce::juce_audio_utils
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)

# --- Headless test of the DSP wrapper:  cmake --build build --target engine_test && build/engine_test ---
add_executable(engine_test EXCLUDE_FROM_ALL src/engine_test.cpp)
target_link_libraries(engine_test PRIVATE superparasites_core)

# --- Level tests of the output stage (Output Level + soft limiter) ---
#   cmake --build build --target level_test && build/level_test            (stage and engine, no JUCE)
#   cmake --build build --target proc_level_test && build/proc_level_test  (the real plug-in processor class, headless)
add_executable(level_test EXCLUDE_FROM_ALL tests/level_test.cpp)
target_link_libraries(level_test PRIVATE superparasites_core)

add_executable(proc_level_test EXCLUDE_FROM_ALL tests/proc_level_test.cpp src/PluginProcessor.cpp src/PluginEditor.cpp)
target_compile_definitions(proc_level_test PRIVATE JucePlugin_Name="Unofficial SuperParasites" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(proc_level_test PRIVATE
  superparasites_core
  juce::juce_audio_processors
  juce::juce_recommended_config_flags)
