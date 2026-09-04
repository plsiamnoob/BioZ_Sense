cmake_minimum_required(VERSION 3.14)
project(AD5940_BioImpedance C ASM)

# Target microcontroller flags
add_compile_options(
    -mcpu=cortex-m3
    -mthumb
    -O2
    -Wall
    -ffunction-sections
    -fdata-sections
)

# Processor definitions required by ADI DFP headers
add_compile_definitions(
    CHIPSEL_594X
    ADUCM3029
    __ADUCM3029__
    __ADUCM302X__
)

# Directory paths
set(EXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/BioZ_Analyzer")
set(DFP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/aducm3029_dfp")
# Target GCC-specific startup and system files
file(GLOB GCC_STARTUP "${DFP_DIR}/Source/GCC/startup_ADuCM3029.*")
set(SYSTEM_SRC "${DFP_DIR}/Source/system_ADuCM3029.c")
set(LINKER_SCRIPT "${DFP_DIR}/Source/GCC/ADuCM3029.ld")

# Check if core_cm3.h exists locally
find_path(CMSIS_CORE_DIR "core_cm3.h"
    PATHS
        "${DFP_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    PATH_SUFFIXES
        CMSIS/Core/Include
        Include/CMSIS/Core/Include
        Include/CMSIS
        CMSIS/Include
        Include
    NO_DEFAULT_PATH
)

# Download ARM CMSIS Core headers via FetchContent if missing locally
if(NOT CMSIS_CORE_DIR)
    message(STATUS "core_cm3.h not found locally. Downloading ARM CMSIS Core headers...")
    include(FetchContent)
    FetchContent_Declare(
        cmsis_core
        GIT_REPOSITORY https://github.com/ARM-software/CMSIS_5.git
        GIT_TAG        5.9.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(cmsis_core)
    set(CMSIS_CORE_DIR "${cmsis_core_SOURCE_DIR}/CMSIS/Core/Include")
endif()

# Only collect sources for the top-level example and the ADICUP3029 port (ignore NUCLEO/STM32)
file(GLOB EXAMPLE_TOP_SRCS "${EXAMPLE_DIR}/*.c")
file(GLOB PORT_SRCS "${EXAMPLE_DIR}/ADICUP3029/*.c")
set(LIB_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/ad5940lib/ad5940.c")

# Executable output definition
add_executable(firmware.elf 
    ${EXAMPLE_TOP_SRCS}
    ${PORT_SRCS} 
    ${LIB_SRCS} 
    ${GCC_STARTUP} 
    ${SYSTEM_SRC}
)

# Include directories
target_include_directories(firmware.elf PRIVATE
    ${EXAMPLE_DIR}
    ${EXAMPLE_DIR}/ADICUP3029
    ${CMAKE_CURRENT_SOURCE_DIR}/ad5940lib
    ${DFP_DIR}/Include
    ${DFP_DIR}/Include/config
    ${DFP_DIR}/Include/drivers
    ${DFP_DIR}/Include/rtos_map
    ${DFP_DIR}/Include/sys
    ${DFP_DIR}/config
    ${CMSIS_CORE_DIR}
)

# Linker script and system call stub configuration
target_link_options(firmware.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-u,_printf_float
    -Wl,-u,_scanf_float
)
# Post-build generation of binary and hex outputs
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex firmware.elf firmware.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary firmware.elf firmware.bin
    COMMAND ${CMAKE_SIZE} firmware.elf
)
