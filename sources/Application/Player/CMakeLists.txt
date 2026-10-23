# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

add_library(application_player
  AudioVoice.cpp
  InsertEffectSlot.cpp
  LFOProcessor.cpp
  Player.cpp
  PlayerChannel.cpp
  PlayerMixer.cpp
  RowFx.cpp
  SelectionResampleSession.cpp
  SyncMaster.cpp
  TablePlayback.cpp
  VoiceADSR.cpp
)

target_link_libraries(application_player PUBLIC
  application_instruments
  application_mixer
  application_audio
  application_model
  application_persistency
  system_filesystem
)

target_include_directories(application_player PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
