# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc. & contributors
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

cmake_minimum_required(VERSION 3.13)

option(BUILD_DESKTOP "Build for Desktop (SDL2)" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json")
option(ENABLE_LOGGING "Enable Trace logging" OFF)
option(ENABLE_PROFILING "Enable profiling" OFF)

if(BUILD_DESKTOP)
  project(picoTrackerAdvance C CXX)

  message(STATUS "Configuring picoTracker Advance for Native Desktop (SDL2)")

  add_compile_options(
    -Wall
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-format
    -Wno-array-bounds
  )

  add_compile_definitions(
    _DESKTOP_
    USE_SDL2
    SDLAUDIO
    ADV
  )

  set(SDL2_DIR "C:/Users/justi/tools/SDL2-2.30.12/x86_64-w64-mingw32" CACHE PATH "Path to SDL2")
  set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
  set(SDL2_LIBRARIES
    "${SDL2_DIR}/lib/libSDL2main.a"
    "${SDL2_DIR}/lib/libSDL2.a"
    mingw32
    winmm
    imm32
    version
    ole32
    oleaut32
    setupapi
  )

  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters/DesktopSDL2/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters/DesktopSDL2/Display
    ${SDL2_INCLUDE_DIRS}
  )

  # Enable ETL debug mode only for Debug builds
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ETL_DEBUG)
  endif()

  add_subdirectory(Foundation)
  add_subdirectory(Services)
  add_subdirectory(System)
  if(ENABLE_LOGGING)
    target_compile_definitions(system_console PRIVATE ENABLE_LOGGING)
  endif()
  add_subdirectory(UIFramework)
  add_subdirectory(Externals)
  add_subdirectory(Application)
  add_subdirectory(Adapters/DesktopSDL2)

  add_executable(picoTrackerAdvance
    Adapters/DesktopSDL2/Main/main.cpp
  )

  target_link_libraries(picoTrackerAdvance PUBLIC
    -Wl,--start-group
    platform_desktop
    application
    application_views
    application_views_baseclasses
    application_views_modaldialogs
    application_model
    application_instruments
    advance_sid
    advance_fm6
    application_mixer
    application_effects
    application_commands
    application_persistency
    application_player
    services_audio
    services_midi
    system_timer
    system_console
    system_process
    system_filesystem
    system_system
    system_io
    profiler
    foundation_services
    foundation
    foundation_variables
    uiframework_basicdatas
    uiframework_interfaces
    uiframework_simplebaseclasses
    tinyxml2
    yxml
    etl
    braids
    opal
    SampleRate::samplerate
    -Wl,--end-group
    ${SDL2_LIBRARIES}
  )

  target_include_directories(picoTrackerAdvance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters/DesktopSDL2/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters/DesktopSDL2/Display
    ${SDL2_INCLUDE_DIRS}
  )

else()
  # Original Hardware STM32 Build
  add_compile_options(
    -Werror
    -Wall
    -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    -fstack-usage
    -fcallgraph-info=su # include above info in callgraph
    -Wstack-usage=2074 # allows high watermark at the time this option was added TODO: reduce this for safety and/or reduce stack size
    
    # braids
    -Wno-unused-variable
    -Wno-array-bounds

    # stm HAL
    -Wno-unused-parameter
    -Wno-missing-field-initializers
  )

  message("picoTracker Advance Build")
  add_definitions(-DADV)
    
  # Enable basic control REPL over serial port on non-Release ADV builds.
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    remove_definitions(-DSERIAL_REPL)
  else()
    add_definitions(-DSERIAL_REPL)
  endif()

  # Enable profiling
  add_compile_definitions(ENABLE_PROFILING=$<BOOL:${ENABLE_PROFILING}>)

  # FreeRTOS config
  add_library(freertos_config INTERFACE)
  target_include_directories(freertos_config SYSTEM
    INTERFACE
    Adapters/adv/config # The config file directory
    Adapters/adv/platform
  )

  target_compile_definitions(freertos_config
    INTERFACE
    projCOVERAGE_TEST=0)
  set(FREERTOS_PORT GCC_ARM_CM7 CACHE STRING "")

  # ------- cmake/gcc-arm-none-eabi.cmake START -------
  set(CMAKE_SYSTEM_NAME               Generic)
  set(CMAKE_SYSTEM_PROCESSOR          arm)

  set(CMAKE_C_COMPILER_ID GNU)
  set(CMAKE_CXX_COMPILER_ID GNU)

  set(TOOLCHAIN_PREFIX                arm-none-eabi-)

  set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc)
  set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
  set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++)
  set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}g++)
  set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy)
  set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size)

  set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
  set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
  set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

  # MCU specific flags
  set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
  set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")

  set(CMAKE_C_FLAGS_DEBUG "-Os -g3")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -g0 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-Os -g3")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0 -DNDEBUG")

  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-use-cxa-atexit")
    
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
  endif()
    
  set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")
  # ------- cmake/gcc-arm-none-eabi.cmake STOP -------

  project(picoTrackerAdvance C CXX ASM)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -include ${CMAKE_CURRENT_SOURCE_DIR}/config/no_heap.h")

  include_directories(${PROJECT_SOURCE_DIR}/Externals)
  add_subdirectory(Externals/FreeRTOS-Kernel)
  add_subdirectory(Externals/FatFs)
  add_subdirectory(Externals/tinyusb)
    
  link_libraries(stm32cubemx freertos_kernel)

  add_subdirectory(Adapters/adv)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ETL_DEBUG)
  endif()

  add_subdirectory(UIFramework)
  add_subdirectory(System)
  if(ENABLE_LOGGING)
    target_compile_definitions(system_console PRIVATE ENABLE_LOGGING)
  endif()

  add_subdirectory(Application)
  add_subdirectory(Externals)
  add_subdirectory(Services)
  add_subdirectory(Foundation)

endif()
