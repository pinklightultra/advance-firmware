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

#ifndef _IMPORT_VIEW_H_
#define _IMPORT_VIEW_H_

#include "Application/Views/BaseClasses/BrowserListingStorage.h"
#include "Application/Views/ScreenView.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/PathUtils.h"
#include "ViewData.h"
#include <string>

class ImportView : public ScreenView {
public:
  ImportView(GUIWindow &w, ViewData *viewData);
  ~ImportView();
  void Reset();
  virtual void ProcessButtonMask(unsigned short mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();

  // Static method to set which view will open the ImportView
  static void SetSourceViewType(ViewType vt);

  // Track which view opened the ImportView (default to project view)
  static ViewType sourceViewType_;

protected:
  void setCurrentFolder(FileSystem *fs, const char *name);
  void warpToNextSample(bool goUp);
  void import();
  void preview();
  void adjustPreviewVolume(int offset);
  void showSampleEditor(etl::string<PFILENAME_SIZE> filename,
                        bool isProjectSample);
  void renameSample(const FileSystemEntry &entry, FileSystem *fs);
  void removeProjectSample(const FileSystemEntry &entry, FileSystem *fs);
  void refreshFileIndexList(FileSystem *fs);

private:
  void onConfirmRenameSample(View &view, ModalView &dialog);
  void onConfirmRemoveProjectSample(View &view, ModalView &dialog);

  size_t topIndex_ = 0;
  size_t currentIndex_ = 0;
  size_t previewPlayingIndex_ = 0;
  short selectedButton_ = 0;
  int toInstr_ = 0;
  bool playKeyHeld_ =
      false; // Flag to track when the play key is being held down
  bool editKeyHeld_ =
      false; // Flag to track when the edit key is being held down
  bool enterKeyHeld_ = false; // Track ENTER key state for deferred dir-enter
  bool pendingDirEnterOnRelease_ = false; // Open dir on ENTER release
  bool inProjectSampleDir_ =
      false; // Flag to track if we're in the project's sample directory
  FileSystemPath::PathString currentDir_;
  char pendingRenamePath_[PFILENAME_SIZE] = {};
  char pendingDeletePath_[PFILENAME_SIZE] = {};
  BrowserFileEntries &fileEntries_;
};
#endif
