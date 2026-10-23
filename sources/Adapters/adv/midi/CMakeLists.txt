# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

add_library(platform_midi
  advMidiService.cpp
  advMidiOutDevice.cpp
  advUSBMidiOutDevice.cpp
  advUSBMidiInDevice.cpp
  advMidiInDevice.cpp
)

target_link_libraries(platform_midi PRIVATE
  platform_usb
)

target_include_directories(platform_midi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
