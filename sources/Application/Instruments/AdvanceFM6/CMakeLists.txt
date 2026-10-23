# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2024 xiphonics, inc.

add_library(advance_fm6
  FM6Voice.cpp
  FM6Dx7Patch.cpp
)

target_include_directories(advance_fm6 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(advance_fm6 PUBLIC
  etl
  system_io
)
