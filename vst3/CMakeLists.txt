cmake_minimum_required(VERSION 3.21)
project(DrumSynthVST3 VERSION 0.1.0)

# Set VST3 SDK path externally, e.g. -DVST3_SDK_ROOT=C:/SDKs/vst3sdk
set(VST3_SDK_ROOT "" CACHE PATH "Path to Steinberg VST3 SDK")
if(NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "Set -DVST3_SDK_ROOT to the Steinberg VST3 SDK root")
endif()

add_subdirectory(${VST3_SDK_ROOT} ${CMAKE_BINARY_DIR}/vst3sdk)

smtg_add_vst3plugin(DrumSynthVST3
  vst3/src/plugin.cpp
  vst3/src/plugin.h
)

target_compile_definitions(DrumSynthVST3 PRIVATE SMTG_MYPLUGINS_NAME=\"DrumSynth\")
target_compile_features(DrumSynthVST3 PRIVATE cxx_std_17)

# Build 32-bit binaries for Windows hosts when using MSVC generator
if(MSVC)
  message(STATUS "Use generator platform Win32 to build x86: -A Win32")
endif()

smtg_target_set_bundle(DrumSynthVST3
  BUNDLE_NAME "DrumSynth"
  COMPANY_NAME "DrumSynth"
)
