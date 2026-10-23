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

#include "InstrumentImportView.h"
#include "Application/AppWindow.h"
#include "Application/Instruments/I_Instrument.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Views/BaseClasses/BrowserEntrySort.h"
#include "Application/Views/BaseClasses/BrowserPathUtils.h"
#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/to_string.h"
#include "ModalDialogs/MessageBox.h"
#include "System/FileSystem/PathUtils.h"
#include <cstring>
#include <memory>
#include <nanoprintf.h>

#define LIST_WIDTH (SCREEN_WIDTH - 2)
// -4 to allow for title, filesize & spacers
#define LIST_PAGE_SIZE (SCREEN_HEIGHT - 4)

namespace {

bool extractLeafName(const char *path,
                     etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> &leafName) {
  FileSystemPath::PathString baseName;
  if (!FileSystemPath::BaseName(path, baseName)) {
    leafName.clear();
    return false;
  }

  leafName = baseName.c_str();
  return !leafName.is_truncated() && !leafName.empty();
}

const FileSystemEntry *
getCurrentEntry(const etl::ivector<FileSystemEntry> &entries,
                const size_t currentIndex) {
  if (currentIndex >= entries.size()) {
    return nullptr;
  }
  return &entries[currentIndex];
}

bool resolveEntryPath(const FileSystemPath::PathString &currentDir,
                      const FileSystemEntry &entry,
                      FileSystemPath::PathString &resolvedPath) {
  if (entry.name == "..") {
    return BrowserPathUtils::ResolveParentDirectory(
        currentDir.c_str(), INSTRUMENTS_DIR, resolvedPath);
  }

  return FileSystemPath::JoinPath(currentDir.c_str(), entry.name.c_str(),
                                  resolvedPath);
}

bool isDirectoryListTruncated(FileSystem *fs) {
  return fs != nullptr &&
         fs->GetLastListDirectoryStatus() == DirectoryListStatus::Truncated;
}

void showDirectoryListTruncatedDialog(View &view) {
  etl::string<24> line("Showing first ");
  etl::to_string(MAX_FILE_INDEX_SIZE, line, true);
  MessageBox *mb =
      MessageBox::Create(view, "Directory too large", line.c_str(), MBBF_OK);
  view.DoModal(mb);
}

} // namespace

InstrumentImportView::InstrumentImportView(GUIWindow &w, ViewData *viewData)
    : ScreenView(w, viewData), fileEntries_(GetInstrumentBrowserFileEntries()) {
  toInstrID_ = viewData_->currentInstrumentID_;
}

InstrumentImportView::~InstrumentImportView() {}

void InstrumentImportView::Reset() {
  topIndex_ = 0;
  currentIndex_ = 0;
  selected_ = 0;
  toInstrID_ = 0;
  currentDir_.clear();
  fileEntries_.clear();
}

void InstrumentImportView::ProcessButtonMask(unsigned short mask,
                                             bool pressed) {
  if (!pressed)
    return;

  if (mask & EPBM_PLAY) {
    // TODO: audition instrument by temporarily loading it and playing a note

    // handle moving up and down the file list
  } else if (mask & EPBM_UP) {
    warpToNextInstrument(true);
  } else if (mask & EPBM_DOWN) {
    warpToNextInstrument(false);
  } else if ((mask & EPBM_LEFT) && (mask & EPBM_NAV)) {
    ViewType vt = VT_INSTRUMENT;
    ViewEvent ve(VET_SWITCH_VIEW, &vt);
    SetChanged();
    NotifyObservers(&ve);
    return;
  } else {
    if (mask & EPBM_ENTER) {
      auto fs = FileSystem::GetInstance();
      const auto *entry = getCurrentEntry(fileEntries_, currentIndex_);
      FileSystemPath::PathString selectedPath;
      if (entry == nullptr) {
        return;
      }

      if (!resolveEntryPath(currentDir_, *entry, selectedPath)) {
        return;
      }

      if (entry->type == PFT_DIR) {
        setCurrentFolder(fs, selectedPath.c_str());
        isDirty_ = true;
        topIndex_ = 0;
      } else {
        importInstrument(selectedPath.c_str());
      }
    }
  }
};

void InstrumentImportView::DrawView() {
  Clear();

  GUITextProperties props;
  GUIPoint pos = GetTitlePosition();

  // Draw title
  setTitle(etl::string<23>("Import Instrument"));

  SetColor(CD_NORMAL);

  int x = 1;
  int y = pos._y + 2;

  for (size_t i = topIndex_;
       i < topIndex_ + LIST_PAGE_SIZE && (i < fileEntries_.size()); i++) {
    if (i == currentIndex_) {
      SetColor(CD_HILITE2);
      props.invert_ = true;
    } else {
      SetColor(CD_NORMAL);
      props.invert_ = false;
    }

    const auto &entry = fileEntries_[i];
    if (entry.type == PFT_DIR) {
      DrawString(x, y, "[", props);
      DrawString(x + 1, y, entry.name.c_str(), props);
      DrawString(x + entry.name.size() + 1, y, "]", props);
    } else {
      DrawString(x, y, entry.name.c_str(), props);
    }
    y++;
  }
};

void InstrumentImportView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {}

void InstrumentImportView::OnFocus() {
  auto fs = FileSystem::GetInstance();
  setCurrentFolder(fs, INSTRUMENTS_DIR);
}

void InstrumentImportView::warpToNextInstrument(bool goUp) {
  if (fileEntries_.empty())
    return;

  if (goUp) {
    if (currentIndex_ > 0) {
      currentIndex_--;
      if (currentIndex_ < topIndex_) {
        topIndex_ = currentIndex_;
      }
    }
  } else {
    if (currentIndex_ < fileEntries_.size() - 1) {
      currentIndex_++;
      if (currentIndex_ >= topIndex_ + LIST_PAGE_SIZE) {
        topIndex_ = currentIndex_ - LIST_PAGE_SIZE + 1;
      }
    }
  }
  isDirty_ = true;
}

void InstrumentImportView::importInstrument(const char *name) {
  etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> leafName;
  if (!extractLeafName(name, leafName)) {
    Trace::Error("INSTRUMENTIMPORT: Failed to extract leaf name from %s", name);
    MessageBox *mb =
        MessageBox::Create(*this, "Invalid instrument file", MBBF_OK);
    DoModal(mb);
    return;
  }

  if (leafName.size() > MAX_INSTRUMENT_FILENAME_LENGTH) {
    Trace::Error("INSTRUMENTIMPORT: Instrument filename exceeds maximum "
                 "length: %s (%zu > %d)",
                 leafName.c_str(), leafName.size(),
                 MAX_INSTRUMENT_FILENAME_LENGTH);

    char sizeMesg[32];
    npf_snprintf(sizeMesg, sizeof(sizeMesg), "Max is %d chars",
                 MAX_INSTRUMENT_FILENAME_LENGTH);
    MessageBox *mb =
        MessageBox::Create(*this, "Filename too long", sizeMesg, MBBF_OK);
    DoModal(mb);
    return;
  }

  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  assert(bank);

  toInstrID_ = viewData_->currentInstrumentID_;

  InstrumentType importedType =
      PersistencyService::GetInstance()->DetectInstrumentType(name);

  if (importedType == IT_NONE) {
    MessageBox *mb =
        MessageBox::Create(*this, "Unknown instrument type", MBBF_OK);
    DoModal(mb);
    return;
  }

  Trace::Log("INSTRUMENTIMPORT", "Detected instrument type: %d", importedType);
  Trace::Log("INSTRUMENTIMPORT", "Importing instrument from file: %s", name);
  Trace::Log("INSTRUMENTIMPORT", "Target instrument ID: %d", toInstrID_);

  I_Instrument *currentInstrument = bank->GetInstrument(toInstrID_);

  if (!currentInstrument) {
    MessageBox *mb = MessageBox::Create(*this, "Invalid instrument", MBBF_OK);
    DoModal(mb);
    return;
  }

  Trace::Log("INSTRUMENTIMPORT",
             "Current instrument type: %d, Imported type: %d",
             currentInstrument->GetType(), importedType);

  if (currentInstrument->GetType() != importedType) {
    Trace::Log("INSTRUMENTIMPORT", "Converting instrument from type %d to %d",
               currentInstrument->GetType(), importedType);

    bank->releaseInstrument(toInstrID_);

    if (bank->GetNextAndAssignID(importedType, toInstrID_) ==
        NO_MORE_INSTRUMENT) {
      MessageBox *mb =
          MessageBox::Create(*this, "Failed to create instrument", MBBF_OK);
      DoModal(mb);
      return;
    }

    Trace::Log("INSTRUMENTIMPORT",
               "Created new instrument of type %d in slot %d", importedType,
               toInstrID_);
  } else {
    Trace::Log("INSTRUMENTIMPORT",
               "Keeping existing instrument of type %d in slot %d",
               importedType, toInstrID_);
  }

  Trace::Log("INSTRUMENTIMPORT", "Created new instrument of type %d",
             importedType);

  I_Instrument *instrument = bank->GetInstrument(toInstrID_);
  PersistencyResult result =
      PersistencyService::GetInstance()->ImportInstrument(instrument, name);

  Trace::Log("INSTRUMENTIMPORT", "Imported TYPE: %d", instrument->GetType());

  if (result == PERSIST_LOADED) {
    instrument->SetChanged();
    instrument->NotifyObservers();

    Variable *channelVar =
        instrument->FindVariable(FourCC::MidiInstrumentChannel);
    if (channelVar) {
      Trace::Log("INSTRUMENTIMPORT", "Final MIDI channel: %d",
                 channelVar->GetInt());
    }

    Trace::Log("INSTRUMENTIMPORT",
               "Final instrument type in importInstrument: %d",
               instrument->GetType());

    viewData_->currentInstrumentID_ = toInstrID_;
    Trace::Log("INSTRUMENTIMPORT",
               "Updated viewData_ currentInstrumentID_ to: %d", toInstrID_);

    MessageBox *mb = MessageBox::Create(*this, "Import successful", MBBF_OK);
    DoModal(mb,
            ModalViewCallback::create<InstrumentImportView,
                                      &InstrumentImportView::onImportSuccess>(
                *this));
  } else {
    MessageBox *mb = MessageBox::Create(*this, "Import failed", MBBF_OK);
    DoModal(mb);
  }

  isDirty_ = true;
}

void InstrumentImportView::onImportSuccess(View &, ModalView &dialog) {
  if (dialog.GetReturnCode() != MBL_OK) {
    return;
  }

  Trace::Log("INSTRUMENTIMPORT",
             "Switching back to instrument view with ID: %d", toInstrID_);

  I_Instrument *instrument =
      viewData_->project_->GetInstrumentBank()->GetInstrument(toInstrID_);
  if (instrument) {
    instrument->SetChanged();
    instrument->NotifyObservers();
  }

  ViewType vt = VT_INSTRUMENT;
  ViewEvent ve(VET_SWITCH_VIEW, &vt);
  SetChanged();
  NotifyObservers(&ve);
}

void InstrumentImportView::setCurrentFolder(FileSystem *fs, const char *name) {
  if (fs == nullptr || name == nullptr) {
    return;
  }

  FileSystemPath::PathString nextDir;
  if (std::strcmp(name, "..") == 0) {
    if (!BrowserPathUtils::ResolveParentDirectory(currentDir_.c_str(),
                                                  INSTRUMENTS_DIR, nextDir)) {
      return;
    }
  } else if (FileSystemPath::IsAbsolutePath(name)) {
    if (!FileSystemPath::NormalizePath(name, nextDir)) {
      Trace::Error("INSTRUMENTIMPORT: Failed to normalize %s", name);
      return;
    }
  } else if (!FileSystemPath::JoinPath(currentDir_.c_str(), name, nextDir)) {
    Trace::Error("INSTRUMENTIMPORT: Failed to resolve %s from %s", name,
                 currentDir_.c_str());
    return;
  }

  fileEntries_.clear();
  const bool listed = fs->ListDirectory(nextDir.c_str(), &fileEntries_,
                                        INSTRUMENT_FILE_EXTENSION, false);
  const bool truncated = isDirectoryListTruncated(fs);
  if (!listed && !truncated) {
    Trace::Error("INSTRUMENTIMPORT: Failed to list %s", nextDir.c_str());
    return;
  }
  if (truncated) {
    Trace::Error("INSTRUMENTIMPORT: Truncated listing for %s", nextDir.c_str());
    showDirectoryListTruncatedDialog(*this);
  }

  SortBrowserEntries(fileEntries_);

  currentDir_ = nextDir;
  currentIndex_ = 0;
  topIndex_ = 0;
  isDirty_ = true;
  BrowserPathUtils::PrependParentEntry(currentDir_.c_str(), INSTRUMENTS_DIR,
                                       &fileEntries_);
  Trace::Debug("loaded %d files from %s", fileEntries_.size(),
               currentDir_.c_str());
}
