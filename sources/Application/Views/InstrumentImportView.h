/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _INSTRUMENT_IMPORT_VIEW_H_
#define _INSTRUMENT_IMPORT_VIEW_H_

#include "Application/Views/BaseClasses/BrowserListingStorage.h"
#include "ScreenView.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/PathUtils.h"
#include "ViewData.h"
#include <string>

class InstrumentImportView : public ScreenView {
public:
  InstrumentImportView(GUIWindow &w, ViewData *viewData);
  ~InstrumentImportView();
  void Reset();
  virtual void ProcessButtonMask(unsigned short mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();

protected:
  void setCurrentFolder(FileSystem *fs, const char *name);
  void warpToNextInstrument(bool goUp);
  void importInstrument(const char *name);

private:
  void onImportSuccess(View &view, ModalView &dialog);

  size_t topIndex_ = 0;
  size_t currentIndex_ = 0;
  short selected_ = 0;
  int toInstrID_ = 0;
  FileSystemPath::PathString currentDir_;
  BrowserFileEntries &fileEntries_;
};
#endif
