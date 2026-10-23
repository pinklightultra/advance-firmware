# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

add_library(application_model
  Chain.cpp
  Config.cpp
  Groove.cpp
  Mixer.cpp
  Phrase.cpp
  Project.cpp
  Scale.cpp
  Song.cpp
  Table.cpp
)

target_link_libraries(application_model PUBLIC
  application_effects
  system_system
  foundation_variables
  application_utils
)

target_include_directories(application_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
