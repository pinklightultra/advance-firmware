# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

add_library(application_views_baseclasses
  BrowserListingStorage.cpp
  I_Action.cpp
  ModalView.cpp
  UIActionField.cpp
  UIBigHexVarField.cpp
  UIField.cpp
  UIFieldInputDispatcher.cpp
  UIIntField.cpp
  UIInsertAmountField.cpp
  UILFOField.cpp
  UIIntVarField.cpp
  UIIntVarOffField.cpp
  UINoteVarField.cpp
  UISortedVarList.cpp
  UIStaticField.cpp
  UITempoField.cpp
  UISwatchField.cpp
  UIBitmaskVarField.cpp
  View.cpp
  ViewEvent.cpp
  ViewUtils.cpp
)

target_link_libraries(application_views_baseclasses PUBLIC
  application_utils
  system_console
)

target_include_directories(application_views_baseclasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
