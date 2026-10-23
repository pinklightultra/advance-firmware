# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

add_library(services_midi
  ControlSurface/ApcMiniMk2Surface.cpp
  ControlSurface/ControlSurfaceDevice.cpp
  ControlSurface/LaunchpadMiniMk3Surface.cpp
  MidiEvent.cpp
  MidiInDevice.cpp
  MidiOutDevice.cpp
  MidiService.cpp
)

target_include_directories(services_midi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
