cmake_minimum_required(VERSION 3.16)
project(BioZ_Sense LANGUAGES C CXX)

# Option to select what to build (can be driven by CMakePresets.json)
option(BUILD_FIRMWARE "Build ADuCM3029 ARM Firmware" ON)
option(BUILD_GUI "Build Qt6 Host GUI" ON)

# ------------------------------------------------------------------------------
# 1. ARM Firmware Target (EVAL-ADICUP3029)
# ------------------------------------------------------------------------------
if(BUILD_FIRMWARE)
    # Check if we are cross-compiling for ARM
    if(CMAKE_SYSTEM_NAME STREQUAL "Generic" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
        message(STATUS "Configuring ARM Firmware build...")
        add_subdirectory(AD5940_Files)
    else()
        message(WARNING "BUILD_FIRMWARE is ON, but host compiler (x86) is active. Skipping ARM subfolder or switch to your ARM preset.")
    endif()
endif()

# ------------------------------------------------------------------------------
# 2. Qt6 Desktop GUI Target (x86_64)
# ------------------------------------------------------------------------------
if(BUILD_GUI)
    # Only configure Qt GUI if not cross-compiling for bare-metal ARM
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
        message(STATUS "Configuring Qt6 GUI App build...")
        add_subdirectory(GUI_App)
    else()
        message(STATUS "Cross-compiling for ARM: Skipping GUI_App build.")
    endif()
endif()