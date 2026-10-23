# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

add_library(application_instruments
  CommandList.cpp
  Filters.cpp
  I_Instrument.cpp
  InstrumentBank.cpp
  InstrumentNameVariable.cpp
  LFOSettings.cpp
  MidiInstrument.cpp
  NoneInstrument.cpp
  SRPUpdaters.cpp
  SampleInstrument.cpp
  SampleSourceInstrument.cpp
  SamplePool.cpp
  SampleVariable.cpp
  SoundSource.cpp
  VoiceParameters.cpp
  WavFile.cpp
  WavFileWriter.cpp
  WavHeader.cpp
)

target_link_libraries(application_instruments PUBLIC
  foundation_services
  system_io
  SampleRate::samplerate
  etl
  application_utils
  profiler
)

target_include_directories(application_instruments PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
