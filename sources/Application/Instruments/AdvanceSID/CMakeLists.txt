# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2022 Mihaly Horvath (original cRSID implementation)
# Copyright (c) 2024 xiphonics, inc.

add_library(advance_sid
  AdvanceSID.cpp
)

target_include_directories(advance_sid PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}
)
