# arm-gcc-toolchain.cmake - CMake toolchain for STM32 using CubeIDE GCC

# Target system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

#MCU version for compiler
add_compile_definitions(STM32G431xx)

# Path to CubeIDE GCC (replace with your path)
set(TOOLCHAIN_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../STM32TOOLS/GCC/bin")

# Compiler
set(CMAKE_C_COMPILER ${TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_BIN}/arm-none-eabi-g++.exe)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe)

# CPU / Architecture flags (example for Cortex-M4)
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS} -O0 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_FLAGS} -O0 -g -Wall -fno-exceptions -fno-rtti")

# Tell CMake not to try to run the compiled test programs
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
