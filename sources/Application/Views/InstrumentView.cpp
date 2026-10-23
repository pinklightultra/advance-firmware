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

#include "InstrumentView.h"
#include "Application/AppWindow.h"
#include "Application/Instruments/MidiInstrument.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Instruments/SampleSourceInstrument.h"
#include "Application/Model/Config.h"
#include "Application/Player/PlayButtonBehavior.h"
#include "Application/Views/ImportView.h"
#include "Application/Views/SampleEditorView.h"
#include "Application/Views/VoiceScreenUtils.h"
#include "BaseClasses/UIBigHexVarField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UIIntVarOffField.h"
#include "BaseClasses/UINoteVarField.h"
#include "BaseClasses/UIStaticField.h"
#include "Externals/etl/include/etl/to_string.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "ModalDialogs/MessageBox.h"
#include "ModalDialogs/TextInputModalView.h"
#include <Application/Utils/stringutils.h>
#include <cstdint>
#include <nanoprintf.h>

static const char *InstrumentTypeDisplayNames[IT_LAST] = {
    "NONE", "SAMPLE", "MIDI", "ORIGINAL SAMPLE"};

static constexpr int kMaxSelectableInstrumentTypeIndex = IT_LAST - 1;
static constexpr InstrumentType kDisplayIndexToInstrumentType[IT_LAST] = {
    IT_NONE, IT_SAMPLE_SOURCE, IT_MIDI, IT_SAMPLE};
static constexpr int kInstrumentTypeToDisplayIndex[IT_LAST] = {0, 3, 1, 2};

constexpr int32_t EmbeddedSlicerGraphXOffset = SampleSlicer::GraphXOffset;
constexpr int32_t EmbeddedSlicerGraphYOffset = 6 * CHAR_HEIGHT;

int displayIndexFromInstrumentType(InstrumentType type) {
  if (type < 0 || type >= IT_LAST) {
    return 0;
  }
  return kInstrumentTypeToDisplayIndex[type];
}

InstrumentView::InstrumentView(GUIWindow &w, ViewData *data)
    : FieldView(w, data), lastFocusWasSliceGraph_(false),
      instrumentType_(FourCC::VarInstrumentType, InstrumentTypeDisplayNames,
                      IT_LAST, 0),
      lastSampleIndex_(-1), pendingSampleSourceLoopModeDirection_(0),
      previewKeyHeld_(false), previewChannel_(0), previewInstrument_(0),
      suppressSampleChangeWarning_(false),
      embeddedSlicer_(
          {GUIPoint(EmbeddedSlicerGraphXOffset, EmbeddedSlicerGraphYOffset),
           GUIPoint(1, 12), false, false, false, true}) {
  project_ = data->project_;
  embeddedSlicer_.AddObserver(*this);

  GUIPoint position = GUIPoint(5, 1);
  typeIntVarField_.emplace_back(position, *&instrumentType_, "Type: %s", 0,
                                kMaxSelectableInstrumentTypeIndex, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*typeIntVarField_.rbegin()));
  (*typeIntVarField_.rbegin()).AddObserver(*this);
  lastFocusID_ = FourCC::VarInstrumentType;

  // Create the name field with the actual instrument variable
  I_Instrument *instr = getInstrument();
  if (instr) {
    // NONE dont have a name field
    if (instr->GetType() != IT_NONE) {
      position._y = 3;
      addNameTextField(instr, position);
    }
  }

  // add ui action fields for exporting and importing instrument settings
  position._y = 2;

  persistentActionField_.emplace_back("Import", FourCC::ActionImport, position);
  fieldList_.insert(fieldList_.end(), &(*persistentActionField_.rbegin()));
  (*persistentActionField_.rbegin()).AddObserver(*this);
  lastFocusID_ = FourCC::ActionImport;

  position._x += 8;
  persistentActionField_.emplace_back("Export", FourCC::ActionExport, position);
  fieldList_.insert(fieldList_.end(), &(*persistentActionField_.rbegin()));
  (*persistentActionField_.rbegin()).AddObserver(*this);
  lastFocusID_ = FourCC::ActionExport;

  sliceCountLabel_.clear();
}

InstrumentView::~InstrumentView() {}

void InstrumentView::Reset() {
  embeddedSlicer_.Reset();
  lastSampleIndex_ = -1;
  suppressSampleChangeWarning_ = false;
  exportInstrument_ = nullptr;
  exportName_.clear();
  lastFocusID_ = FourCC::VarInstrumentType;
  lastFocusWasSliceGraph_ = false;
  pendingSampleSourceLoopModeDirection_ = 0;
  instrumentType_.SetInt(0, false);
  sliceCountLabel_.clear();
}

static void updateSliceCountLabel(etl::string<20> &label,
                                  I_Instrument *instrument) {
  int32_t count = 0;
  if (instrument && instrument->GetType() == IT_SAMPLE) {
    SampleInstrument *sampleInstrument =
        static_cast<SampleInstrument *>(instrument);
    for (size_t i = 0; i < SampleInstrument::MaxSlices; ++i) {
      if (sampleInstrument->IsSliceDefined(i)) {
        count++;
      }
    }
  } else if (instrument && instrument->GetType() == IT_SAMPLE_SOURCE) {
    SampleSourceInstrument *sampleSourceInstrument =
        static_cast<SampleSourceInstrument *>(instrument);
    for (size_t i = 0; i < SampleSourceInstrument::MaxSlices; ++i) {
      if (sampleSourceInstrument->IsSliceDefined(i)) {
        count++;
      }
    }
  }
  if (count <= 1) {
    label = "Slices: off";
  } else {
    label = "Slices: ";
    etl::format_spec format;
    format.width(2).fill(' ');
    etl::to_string(count, label, format, true);
  }
}

static bool
resetSampleSourceOscillatorIfSampleTooLarge(I_Instrument *instrument) {
  if (!instrument || instrument->GetType() != IT_SAMPLE_SOURCE) {
    return false;
  }

  SampleSourceInstrument *sampleSourceInstrument =
      static_cast<SampleSourceInstrument *>(instrument);
  return sampleSourceInstrument->ResetOscillatorLoopModeIfSampleTooLarge();
}

static bool skipSampleSourceOscillatorIfSampleTooLarge(I_Instrument *instrument,
                                                       int direction) {
  if (!instrument || instrument->GetType() != IT_SAMPLE_SOURCE) {
    return false;
  }

  SampleSourceInstrument *sampleSourceInstrument =
      static_cast<SampleSourceInstrument *>(instrument);
  return sampleSourceInstrument->SkipOscillatorLoopModeIfSampleTooLarge(
      direction);
}

void InstrumentView::addNameTextField(I_Instrument *instr, GUIPoint &position) {
  nameVariables_.emplace_back(instr);
  Variable &nameVar = *nameVariables_.rbegin();

  auto label =
      etl::make_string_with_capacity<MAX_UITEXTFIELD_LABEL_LENGTH>("Name: ");

  // Use an empty default name - we don't want to populate with sample filename
  // The display name will still be shown on the phrase screen via
  // GetDisplayName()
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> defaultName;

  nameTextField_.emplace_back(nameVar, position, label, FourCC::InstrumentName,
                              defaultName);
  fieldList_.insert(fieldList_.end(), &(*nameTextField_.rbegin()));
}

I_Instrument *InstrumentView::getInstrument() {
  int id = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  return bank->GetInstrument(id);
};

void InstrumentView::onInstrumentTypeChange(bool updateUI) {
  auto nuType = kDisplayIndexToInstrumentType[instrumentType_.GetInt()];
  I_Instrument *old = getInstrument();

  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  auto id = viewData_->currentInstrumentID_;
  // release prev instrument back to available pool
  if (old != nullptr) {
    // first check if the instrument type actually changed, because could be
    // user is at end of instrument list and just keeps pressing key combo to
    // trigger next instrument event again and again
    if (old->GetType() == nuType) {
      if (updateUI) {
        refreshInstrumentFields();
      }
      return;
    }
    if (old->GetType() == IT_SAMPLE_SOURCE) {
      clearEmbeddedSlicerWaveform();
      embeddedSlicer_.Reset();
    }
    bank->releaseInstrument(viewData_->currentInstrumentID_);
  }

  // now assign new instrument type to the current instrument slot id
  unsigned short result = bank->GetNextAndAssignID(nuType, id);

  if (result == NO_MORE_INSTRUMENT) {
    Trace::Error("INSTRUMENTVIEW", "Failed to assign new instrument type: %d",
                 nuType);

    // Try to find the next available instrument type
    bool found = false;
    for (int i = displayIndexFromInstrumentType(nuType) + 1;
         i <= kMaxSelectableInstrumentTypeIndex; i++) {
      InstrumentType nextType = kDisplayIndexToInstrumentType[i];
      result = bank->GetNextAndAssignID(nextType, id);
      if (result != NO_MORE_INSTRUMENT) {
        Trace::Log("INSTRUMENTVIEW", "Assigned next available type: %d",
                   nextType);
        instrumentType_.SetInt(i, false);
        found = true;
        break; // Exit loop on success
      }
    }

    if (!found) {
      Trace::Log("INSTRUMENTVIEW",
                 "No other instrument types available, setting to NONE");
      instrumentType_.SetInt(displayIndexFromInstrumentType(IT_NONE), false);
    }

    refreshInstrumentFields();
    isDirty_ = true;
    return;
  }

  // Get the new instrument after type change
  I_Instrument *newInstr = getInstrument();
  if (newInstr) {
    Trace::Log("INSTRUMENTVIEW", "New instrument type: %d",
               newInstr->GetType());
  }

  // Refresh the UI fields for the new instrument type
  refreshInstrumentFields();

  // Mark the view as dirty to ensure it gets redrawn
  isDirty_ = true;
}

void InstrumentView::applyProposedTypeChangeUI() {
  instrumentType_.SetInt(displayIndexFromInstrumentType(pendingInstrumentType_),
                         false);
  onInstrumentTypeChange();
}

void InstrumentView::onInstrumentChange() {

  ClearFocus();

  I_Instrument *old = getInstrument();
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  if (getInstrument() != old) {
    getInstrument()->RemoveObserver(*this);
  };

  // update type field to match current instrument
  ((WatchedVariable *)&instrumentType_)
      ->SetInt(displayIndexFromInstrumentType(getInstrument()->GetType()));

  refreshInstrumentFields();
};

void InstrumentView::refreshInstrumentFields() {
  for (auto &f : intVarField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : bigHexVarField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : intVarOffField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : bitmaskVarField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : sampleActionField_) {
    f.RemoveObserver(*this);
  }

  fieldList_.clear();
  intVarField_.clear();
  noteVarField_.clear();
  staticField_.clear();
  bigHexVarField_.clear();
  intVarOffField_.clear();
  sampleActionField_.clear();
  bitmaskVarField_.clear();
  nameTextField_.clear();
  nameVariables_.clear();
  lastSampleIndex_ = -1;
  InstrumentType currentType =
      getInstrument() ? getInstrument()->GetType() : IT_NONE;
  if (currentType != IT_SAMPLE_SOURCE) {
    clearEmbeddedSlicerWaveform();
    embeddedSlicer_.Reset();
  }

  // first put back the type field as its shown on *all* instrument types
  fieldList_.insert(fieldList_.end(), &(*typeIntVarField_.rbegin()));
  lastFocusID_ = FourCC::VarInstrumentType;

  // Re-add the action fields for export and import only if not IT_NONE
  if (currentType != IT_NONE) {
    for (auto &action : persistentActionField_) {
      fieldList_.insert(fieldList_.end(), &action);
      action.AddObserver(*this); // Make sure observers are re-added
    }
  } else {
    // add back only the import field for IT_NONE
    // bit of a hack !!since we just assume that import is the first action
    // field
    fieldList_.insert(fieldList_.end(), &(*persistentActionField_.begin()));
    (*persistentActionField_.rbegin()).AddObserver(*this);
  }

  // Create a new nameTextField_ if the instrument type supports it
  if (currentType != IT_NONE) {
    I_Instrument *instr = getInstrument();
    if (instr) {
      GUIPoint position = GetAnchor();
      addNameTextField(instr, position);
    }
  }

  InstrumentType it = getInstrument()->GetType();
  switch (it) {
  case IT_NONE:
    fillNoneParameters();
    break;
  case IT_MIDI:
    fillMidiParameters();
    break;
  case IT_SAMPLE:
    fillSampleParameters();
    break;
  case IT_SAMPLE_SOURCE:
    fillSampleSourceParameters();
    break;
  case IT_LAST:
    // NA
    break;
  };

  for (auto field : fieldList_) {
    if (embeddedSlicer_.HasGraphFocus(field)) {
      if (lastFocusWasSliceGraph_) {
        SetFocus(field);
        break;
      }
      continue;
    }
    if (!lastFocusWasSliceGraph_ &&
        ((UIIntVarField *)field)->GetVariableID() == lastFocusID_) {
      SetFocus(field);
      break;
    }
  }

  // observer all var fields so we can mark the instrument as modified
  // to be able to show confirmation dialog when switching instrument type
  for (auto &f : intVarField_) {
    f.AddObserver(*this);
  }
  for (auto &f : bigHexVarField_) {
    f.AddObserver(*this);
  }
  for (auto &f : intVarOffField_) {
    f.AddObserver(*this);
  }
  for (auto &f : bitmaskVarField_) {
    f.AddObserver(*this);
  }

  getInstrument()->AddObserver(*this);
}

void InstrumentView::fillNoneParameters() {}

void InstrumentView::fillSampleParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  SampleInstrument *instrument = (SampleInstrument *)instr;
  lastSampleIndex_ = instrument->GetSampleIndex();

  GUIPoint position = GetAnchor();
  const int baseX = position._x;

  // offset y to account for instrument type and export/import fields
  position._y += 1;

  Variable *v = instrument->FindVariable(FourCC::SampleInstrumentSample);
  SamplePool *sp = SamplePool::GetInstance();
  intVarField_.emplace_back(position, *v, "Sample: %.17s", 0,
                            sp->GetNameListSize() - 1, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  updateSliceCountLabel(sliceCountLabel_, instrument);
  staticField_.emplace_back(position, sliceCountLabel_.c_str());
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  GUIPoint actionPos = position;
  actionPos._x = baseX + 12;
  sampleActionField_.emplace_back("Adjust", FourCC::ActionShowSampleSlices,
                                  actionPos);
  fieldList_.insert(fieldList_.end(), &sampleActionField_.back());
  sampleActionField_.back().AddObserver(*this);

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentVolume);
  intVarField_.emplace_back(position, *v, "Volume: %d [%2.2X]", 0, 255, 1, 10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentPan);
  intVarField_.emplace_back(position, *v, "Pan: %2.2X", 0, 0xFE, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentRootNote);
  noteVarField_.emplace_back(position, *v, "Root Note: %s", 0, 0x7F, 1, 0x0C);
  fieldList_.insert(fieldList_.end(), &(*noteVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentFineTune);
  intVarField_.emplace_back(position, *v, "Detune: %2.2X", 0, 255, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentCrushVolume);
  intVarField_.emplace_back(position, *v, "Drive: %2.2X", 0, 0xFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentCrush);
  intVarField_.emplace_back(position, *v, "Crush: %d", 1, 0x10, 1, 4);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentDownsample);
  intVarField_.emplace_back(position, *v, "Downsample: %d", 0, 8, 1, 4);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 2;
  staticField_.emplace_back(position, "Flt Cut/Res:");
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));

  position._x += 13;
  v = instrument->FindVariable(FourCC::SampleInstrumentFilterCutOff);
  intVarField_.emplace_back(position, *v, "%2.2X", 0, 0xFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._x += 3;
  v = instrument->FindVariable(FourCC::SampleInstrumentFilterResonance);
  intVarField_.emplace_back(position, *v, "%2.2X", 0, 0xFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._x -= 16;

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentFilterType);
  intVarField_.emplace_back(position, *v, "Type: %2.2X", 0, 0xFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentFilterMode);
  intVarField_.emplace_back(position, *v, "Mode: %s", 0, 2, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentInterpolation);
  intVarField_.emplace_back(position, *v, "Interpolation: %s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentLoopMode);
  intVarField_.emplace_back(position, *v, "Loop Mode: %s", 0, SILM_LAST - 1, 1,
                            1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentStart);
  bigHexVarField_.emplace_back(position, *v, 7, "Start: %7.7X", 0,
                               instrument->GetSampleSize() - 1, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentLoopStart);
  bigHexVarField_.emplace_back(position, *v, 7, "Loop Start: %7.7X", 0,
                               instrument->GetSampleSize() - 1, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentEnd);
  bigHexVarField_.emplace_back(position, *v, 7, "Loop End: %7.7X", 0,
                               instrument->GetSampleSize() - 1, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::SampleInstrumentTable);
  intVarOffField_.emplace_back(position, *v, "Table: %2.2X", 0x00,
                               TABLE_COUNT - 1, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarOffField_.rbegin()));

  v = instrument->FindVariable(FourCC::SampleInstrumentTableAutomation);
  position._x += 12;
  intVarField_.emplace_back(position, *v, "Auto: %s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
};

void InstrumentView::fillSampleSourceParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instrument = bank->GetInstrument(i);
  SampleSourceInstrument *sampleSourceInstrument =
      static_cast<SampleSourceInstrument *>(instrument);
  lastSampleIndex_ = sampleSourceInstrument->GetSampleIndex();

  GUIPoint position = GetAnchor();

  position._y += 1;

  Variable *v = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentSample);
  SamplePool *sp = SamplePool::GetInstance();
  intVarField_.emplace_back(position, *v, "Sample: %.17s", 0,
                            sp->GetNameListSize() - 1, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  setupEmbeddedSlicer();
  embeddedSlicer_.MarkModalRecovery(2);
  ((AppWindow &)w_).SetDirty();
  addEmbeddedSlicerFields(position);

  GUIPoint leftColumn(1, 14);
  GUIPoint rightColumn = leftColumn;
  rightColumn._x = 17;

  position = leftColumn;
  v = sampleSourceInstrument->FindVariable(FourCC::SampleSourceInstrumentLevel);
  intVarField_.emplace_back(position, *v, "Level: %d [%2.2X]", 0, 255, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentRootNote);
  noteVarField_.emplace_back(position, *v, "Root Note: %s", 0, 0x7F, 1, 0x0C);
  fieldList_.insert(fieldList_.end(), &(*noteVarField_.rbegin()));

  position._y += 1;
  v = sampleSourceInstrument->FindVariable(FourCC::SampleSourceInstrumentTable);
  intVarOffField_.emplace_back(position, *v, "Table: %2.2X", 0x00,
                               TABLE_COUNT - 1, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarOffField_.rbegin()));

  v = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentTableAutomation);
  position._y += 1;
  intVarField_.emplace_back(position, *v, "Auto: %s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position = rightColumn;
  v = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentLoopMode);
  intVarField_.emplace_back(position, *v, "Mode: %.8s", 0, SSLM_LAST - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = sampleSourceInstrument->FindVariable(FourCC::SampleSourceInstrumentStart);
  bigHexVarField_.emplace_back(position, *v, 7, "Start: %7.7X", 0,
                               sampleSourceInstrument->GetSampleSize() - 1, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));

  position._y += 1;
  v = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentLoopStart);
  bigHexVarField_.emplace_back(position, *v, 7, "Loop:  %7.7X", 0,
                               sampleSourceInstrument->GetSampleSize() - 1, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));

  position._y += 1;
  v = sampleSourceInstrument->FindVariable(FourCC::SampleSourceInstrumentEnd);
  bigHexVarField_.emplace_back(position, *v, 7, "End:   %7.7X", 0,
                               sampleSourceInstrument->GetSampleSize() - 1, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));
}

void InstrumentView::clearEmbeddedSlicerWaveform() {
  if (!isEmbeddedSlicerActive()) {
    return;
  }

  embeddedSlicer_.MarkModalRecovery(0);
  embeddedSlicer_.ClearWaveformRegion(*this);
  ((AppWindow &)w_).InvalidateTextCache();
  ((AppWindow &)w_).SetDirty();
}

void InstrumentView::setupEmbeddedSlicer() {
  I_Instrument *instrument = getInstrument();
  if (!instrument || instrument->GetType() != IT_SAMPLE_SOURCE) {
    embeddedSlicer_.Reset();
    return;
  }

  embeddedSlicer_.Attach(instrument,
                         static_cast<int32_t>(viewData_->currentInstrumentID_));
  updateEmbeddedSlicerPreviewRange();
}

void InstrumentView::addEmbeddedSlicerFields(GUIPoint &position) {
  embeddedSlicer_.AddFields(fieldList_, *this);
  position._x = GetAnchor()._x;
}

bool InstrumentView::isEmbeddedSlicerActive() const {
  return embeddedSlicer_.IsActive();
}

bool InstrumentView::startInstrumentPreview(unsigned short mask) {
  if (!IsSamplePreviewButtonMask(mask, EPBM_PLAY, EPBM_ALT)) {
    return false;
  }

  I_Instrument *instrument = getInstrument();
  if (!instrument || !viewData_->song_) {
    return false;
  }
  if (instrument->GetType() != IT_SAMPLE_SOURCE) {
    return false;
  }
  SampleSourceInstrument *sampleSourceInstrument =
      static_cast<SampleSourceInstrument *>(instrument);

  if (previewKeyHeld_) {
    return true;
  }

  int channelCount = viewData_->song_->GetChannelCount();
  if (channelCount <= 0) {
    return true;
  }

  previewInstrument_ =
      static_cast<unsigned short>(viewData_->currentInstrumentID_);
  previewChannel_ = static_cast<unsigned short>(channelCount - 1);
  Player::GetInstance()->PlayNote(
      previewInstrument_, previewChannel_,
      embeddedSlicer_.IsActive() ? embeddedSlicer_.PreviewNote()
                                 : sampleSourceInstrument->GetPreviewNote(),
      0x7F);
  previewKeyHeld_ = true;
  return true;
}

void InstrumentView::stopInstrumentPreview() {
  if (!previewKeyHeld_) {
    return;
  }

  Player::GetInstance()->StopNote(previewInstrument_, previewChannel_);
  previewKeyHeld_ = false;
}

void InstrumentView::drawEmbeddedSlicerWaveform() {
  if (!isEmbeddedSlicerActive() || HasModalView()) {
    return;
  }
  uint32_t sample = 0;
  embeddedSlicer_.SetFocusSampleHint(focusedEmbeddedSlicerPointSample(sample),
                                     sample);
  embeddedSlicer_.SetPreviewFocusHint(focusedEmbeddedSlicerPreviewField());
  updateEmbeddedSlicerPreviewRange();
  embeddedSlicer_.Draw(*this);
}

void InstrumentView::AutoSliceConfirmCallback(View &view, ModalView &dialog) {
  auto &self = static_cast<InstrumentView &>(view);
  self.embeddedSlicer_.MarkModalRecovery(2);
  self.isDirty_ = true;
  ((AppWindow &)self.w_).SetDirty();
  if (dialog.GetReturnCode() != MBL_YES) {
    return;
  }
  self.embeddedSlicer_.AutoSliceEvenly(self);
  I_Instrument *instrument = self.getInstrument();
  updateSliceCountLabel(self.sliceCountLabel_, instrument);
}

bool InstrumentView::focusedEmbeddedSlicerPointSample(uint32_t &sample) {
  if (!isEmbeddedSlicerActive() || embeddedSlicer_.SampleSize() == 0) {
    return false;
  }

  UIField *focus = GetFocus();
  for (auto &field : bigHexVarField_) {
    if (&field != focus) {
      continue;
    }

    FourCC id = field.GetVariableID();
    if (id != FourCC::SampleSourceInstrumentStart &&
        id != FourCC::SampleSourceInstrumentLoopStart &&
        id != FourCC::SampleSourceInstrumentEnd) {
      return false;
    }

    int32_t value = field.GetVariable().GetInt();
    sample = value > 0 ? static_cast<uint32_t>(value) : 0;
    if (sample >= embeddedSlicer_.SampleSize()) {
      sample = embeddedSlicer_.SampleSize() - 1;
    }
    return true;
  }
  return false;
}

bool InstrumentView::focusedEmbeddedSlicerPreviewField() {
  if (!isEmbeddedSlicerActive()) {
    return false;
  }

  UIField *focus = GetFocus();
  for (auto &field : intVarField_) {
    if (&field == focus &&
        field.GetVariableID() == FourCC::SampleSourceInstrumentLoopMode) {
      return true;
    }
  }
  return false;
}

void InstrumentView::updateEmbeddedSlicerPreviewRange() {
  if (!isEmbeddedSlicerActive() || embeddedSlicer_.SampleSize() == 0) {
    embeddedSlicer_.SetPreviewRangeHint(false, 0, 0, 0, false);
    return;
  }

  I_Instrument *instrument = getInstrument();
  if (!instrument || instrument->GetType() != IT_SAMPLE_SOURCE ||
      embeddedSlicer_.HasSlicesForPlayback()) {
    embeddedSlicer_.SetPreviewRangeHint(false, 0, 0, 0, false);
    return;
  }

  SampleSourceInstrument *sampleSourceInstrument =
      static_cast<SampleSourceInstrument *>(instrument);
  Variable *startVar =
      sampleSourceInstrument->FindVariable(FourCC::SampleSourceInstrumentStart);
  Variable *loopModeVar = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentLoopMode);
  Variable *loopStartVar = sampleSourceInstrument->FindVariable(
      FourCC::SampleSourceInstrumentLoopStart);
  Variable *endVar =
      sampleSourceInstrument->FindVariable(FourCC::SampleSourceInstrumentEnd);
  if (!startVar || !loopModeVar || !loopStartVar || !endVar) {
    embeddedSlicer_.SetPreviewRangeHint(false, 0, 0, 0, false);
    return;
  }

  int32_t startValue = startVar->GetInt();
  int32_t loopStartValue = loopStartVar->GetInt();
  int32_t endValue = endVar->GetInt();
  uint32_t start = startValue > 0 ? static_cast<uint32_t>(startValue) : 0;
  uint32_t loopStart =
      loopStartValue > 0 ? static_cast<uint32_t>(loopStartValue) : 0;
  uint32_t end = endValue > 0 ? static_cast<uint32_t>(endValue) : 0;
  bool loopStartVisible = SampleSlicer::ShouldShowPreviewLoopStart(
      true,
      SampleSourceInstrument::PlaybackModeUsesLoopPoint(loopModeVar->GetInt()));
  embeddedSlicer_.SetPreviewRangeHint(true, start, loopStart, end,
                                      loopStartVisible);
}

void InstrumentView::fillMidiParameters() {

  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  MidiInstrument *instrument = (MidiInstrument *)instr;
  GUIPoint position = GetAnchor();

  // offset y to account for instrument type, name and export/import fields
  position._y += 3;

  Variable *v = instrument->FindVariable(FourCC::MidiInstrumentChannel);
  intVarField_.emplace_back(
      UIIntVarField(position, *v, "Channel: %2.2d", 0, 0x0F, 1, 0x04, 1));
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::MidiInstrumentVolume);
  intVarField_.emplace_back(
      UIIntVarField(position, *v, "Volume: %2.2X", 0, 0xFF, 1, 0x10));
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::MidiInstrumentNoteLength);
  intVarField_.emplace_back(
      UIIntVarField(position, *v, "Length: %2.2X", 0, 0xFF, 1, 0x10));
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::MidiInstrumentProgram);
  intVarOffField_.emplace_back(
      UIIntVarOffField(position, *v, "Program: %2.2X", 0, 0x7F, 1, 0x10));
  fieldList_.insert(fieldList_.end(), &(*intVarOffField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::MidiInstrumentTableAutomation);
  intVarField_.emplace_back(
      UIIntVarField(position, *v, "Automation: %s", 0, 1, 1, 1));
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position._y += 1;
  v = instrument->FindVariable(FourCC::MidiInstrumentTable);
  intVarOffField_.emplace_back(
      UIIntVarOffField(position, *v, "Table: %2.2X", 0, 0x7F, 1, 0x10));
  fieldList_.insert(fieldList_.end(), &(*intVarOffField_.rbegin()));
};

void InstrumentView::warpToNext(int offset) {
  int instrument = viewData_->currentInstrumentID_ + offset;
  if (instrument >= MAX_INSTRUMENT_COUNT) {
    instrument = instrument - MAX_INSTRUMENT_COUNT;
  };
  if (instrument < 0) {
    instrument = MAX_INSTRUMENT_COUNT + instrument;
  };
  viewData_->currentInstrumentID_ = instrument;
  onInstrumentChange();
  isDirty_ = true;
};

void InstrumentView::ProcessButtonMask(unsigned short mask, bool pressed) {

  if (!pressed) {
    if (previewKeyHeld_ &&
        !IsSamplePreviewComboHeld(mask, EPBM_PLAY, EPBM_ALT)) {
      stopInstrumentPreview();
      return;
    }
    if (embeddedSlicer_.ProcessButtonMask(*this, *viewData_, mask, pressed)) {
      return;
    }
    return;
  }

  isDirty_ = false;
  const bool embeddedSlicerActive = isEmbeddedSlicerActive();

  if (startInstrumentPreview(mask)) {
    return;
  }

  uint32_t focusSample = 0;
  embeddedSlicer_.SetFocusSampleHint(
      focusedEmbeddedSlicerPointSample(focusSample), focusSample);
  embeddedSlicer_.SetPreviewFocusHint(focusedEmbeddedSlicerPreviewField());
  updateEmbeddedSlicerPreviewRange();
  if (embeddedSlicer_.ProcessButtonMask(*this, *viewData_, mask, pressed)) {
    return;
  }

  if ((mask & EPBM_EDIT) && (mask & EPBM_ENTER)) {
    int i = viewData_->currentInstrumentID_;
    InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
    I_Instrument *instr = bank->GetInstrument(i);
    if (GetFocus() == *fieldList_.begin()) {
      bool instrumentModified = checkInstrumentModified();
      if (instrumentModified) {
        MessageBox *mb = MessageBox::Create(*this, "Reset all settings?",
                                            MBBF_YES | MBBF_NO);
        pendingPurgeInstrument_ = instr;
        DoModal(mb,
                ModalViewCallback::create<
                    InstrumentView, &InstrumentView::onConfirmResetInstrument>(
                    *this));
      }
      return;
    }
    if (getInstrument()->GetType() == IT_SAMPLE ||
        getInstrument()->GetType() == IT_SAMPLE_SOURCE) {
      UIField *focus = GetFocus();
      UIIntVarField *field = embeddedSlicer_.HasGraphFocus(focus)
                                 ? nullptr
                                 : (UIIntVarField *)focus;
      Variable *endVar = nullptr;
      int sampleSize = 0;
      if (instr->GetType() == IT_SAMPLE) {
        SampleInstrument *sampleInstrument =
            static_cast<SampleInstrument *>(instr);
        endVar = sampleInstrument->FindVariable(FourCC::SampleInstrumentEnd);
        sampleSize = sampleInstrument->GetSampleSize();
      } else if (instr->GetType() == IT_SAMPLE_SOURCE) {
        SampleSourceInstrument *sampleSourceInstrument =
            static_cast<SampleSourceInstrument *>(instr);
        endVar = sampleSourceInstrument->FindVariable(
            FourCC::SampleSourceInstrumentEnd);
        sampleSize = sampleSourceInstrument->GetSampleSize();
      }
      if (endVar && field && field->GetVariableID() == endVar->GetID()) {
        Variable &var = field->GetVariable();
        var.SetInt(sampleSize - 1);
        isDirty_ = true;
        return;
      }
    }
  }

  // Call the parent class's implementation first to ensure action fields like
  // Export, Import work correctly
  pendingSampleSourceLoopModeDirection_ = 0;
  if ((mask & EPBM_ENTER) &&
      lastFocusID_ == FourCC::SampleSourceInstrumentLoopMode) {
    if ((mask & EPBM_UP) || (mask & EPBM_RIGHT)) {
      pendingSampleSourceLoopModeDirection_ = 1;
    }
    if ((mask & EPBM_DOWN) || (mask & EPBM_LEFT)) {
      pendingSampleSourceLoopModeDirection_ = -1;
    }
  }
  FieldView::ProcessButtonMask(mask, pressed);
  pendingSampleSourceLoopModeDirection_ = 0;
  if (embeddedSlicerActive) {
    uint32_t pointSample = 0;
    bool hasPointSample = focusedEmbeddedSlicerPointSample(pointSample);
    embeddedSlicer_.SetFocusSampleHint(hasPointSample, pointSample);
    embeddedSlicer_.SetPreviewFocusHint(focusedEmbeddedSlicerPreviewField());
    updateEmbeddedSlicerPreviewRange();
    if (hasPointSample && embeddedSlicer_.UpdateZoomWindow()) {
      embeddedSlicer_.InvalidateWaveform();
      embeddedSlicer_.Graph().RequestFullRedraw();
      embeddedSlicer_.UpdateLabels();
      isDirty_ = true;
    }
  }

  Player *player = Player::GetInstance();

  if (mask == EPBM_ENTER) {
    // Get the current field to check if we're on the sample field
    UIField *focus = GetFocus();
    UIIntVarField *currentField =
        embeddedSlicer_.HasGraphFocus(focus) ? nullptr : (UIIntVarField *)focus;

    // Only allow sample import when the sample field is selected
    Variable *sampleVar = nullptr;
    if (getInstrument()->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstrument =
          static_cast<SampleInstrument *>(getInstrument());
      sampleVar =
          sampleInstrument->FindVariable(FourCC::SampleInstrumentSample);
    } else if (getInstrument()->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *sampleSourceInstrument =
          static_cast<SampleSourceInstrument *>(getInstrument());
      sampleVar = sampleSourceInstrument->FindVariable(
          FourCC::SampleSourceInstrumentSample);
    }
    if (sampleVar && currentField &&
        currentField->GetVariableID() == sampleVar->GetID()) {

      if (viewMode_ == VM_NEW) {
        viewMode_ = VM_NORMAL; // clear the "enter double tap" state

        // First check if the samplelib exists
        bool samplelibExists =
            FileSystem::GetInstance()->exists(SAMPLES_LIB_DIR);

        if (!samplelibExists) {
          MessageBox *mb =
              MessageBox::Create(*this, "Can't access the samplelib", MBBF_OK);
          DoModal(mb);
        } else {
          ImportView::SetSourceViewType(VT_INSTRUMENT);
          // set browser into sample import mode in top level samples dir
          viewData_->importViewStartDir = SAMPLES_LIB_DIR;

          // Go to import sample
          ViewType vt = VT_IMPORT;
          ViewEvent ve(VET_SWITCH_VIEW, &vt);
          SetChanged();
          NotifyObservers(&ve);
        }
      } else {
        // mark as "new" mode so a 2nd following ENTER will trigger the sample
        // import above
        viewMode_ = VM_NEW;
      }
    } else if (viewMode_ == VM_NEW) {
      // If we're not on the sample field but in VM_NEW mode, reset it
      viewMode_ = VM_NORMAL;
    }

    UIIntVarField *field = currentField;
    if (field) {
      Variable &v = field->GetVariable();
      switch (v.GetID()) {
      case FourCC::SampleInstrumentTable:
      case FourCC::SampleSourceInstrumentTable: {
        int next = TableHolder::GetInstance()->GetNext();
        if (next != NO_MORE_TABLE) {
          v.SetInt(next);
          isDirty_ = true;
        }
        break;
      }
      default:
        break;
      }
    }
    mask &= (0xFFFF - EPBM_ENTER);
  } else {
    // Clear the VM_NEW state if any key other than ENTER is pressed
    if (viewMode_ == VM_NEW) {
      viewMode_ = VM_NORMAL;
    }
  }

  if (viewMode_ == VM_CLONE) {
    if ((mask & EPBM_ENTER) && (mask & EPBM_ALT)) {
      UIField *focus = GetFocus();
      UIIntVarField *field = embeddedSlicer_.HasGraphFocus(focus)
                                 ? nullptr
                                 : (UIIntVarField *)focus;
      mask &= (0xFFFF - EPBM_ENTER);
      if (!field) {
        return;
      }
      Variable &v = field->GetVariable();
      int current = v.GetInt();
      if (current == -1)
        return;

      if ((field->GetVariableID() == FourCC::SampleInstrumentTable) ||
          (field->GetVariableID() == FourCC::SampleSourceInstrumentTable) ||
          (field->GetVariableID() == FourCC::MidiInstrumentTable)) {
        int next = TableHolder::GetInstance()->Clone(current);
        if (next != NO_MORE_TABLE) {
          v.SetInt(next);
          isDirty_ = true;
        }
      };
    }
    mask &= (0xFFFF - (EPBM_ENTER | EPBM_ALT));
  };

  if (viewMode_ == VM_SELECTION) {
  } else {
    // viewMode_ = VM_NORMAL;
  }

  // EDIT Modifier
  if (mask & EPBM_EDIT) {
    if (mask & EPBM_LEFT)
      warpToNext(-1);
    if (mask & EPBM_RIGHT)
      warpToNext(+1);
    if (mask & EPBM_DOWN)
      warpToNext(-16);
    if (mask & EPBM_UP)
      warpToNext(+16);
    if (mask & EPBM_ALT) {
      viewMode_ = VM_CLONE;
    };
    if (mask & EPBM_PLAY) {
      // recording screen
      if (!Player::GetInstance()->IsRunning()) {
        switchToRecordView();
      }
    }
  } else if (mask & EPBM_NAV) {
    // NAV Modifier
    if (mask & EPBM_LEFT) {
      ViewType vt = VT_PHRASE;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);

      // remove listening when leaving this screen
      getInstrument()->RemoveObserver(*this);
      ((WatchedVariable *)&instrumentType_)->RemoveObserver(*this);

      SetChanged();
      NotifyObservers(&ve);
    }

    if ((mask & EPBM_RIGHT) && SupportsAudioVoiceScreen(getInstrument())) {
      ViewType vt = VT_VOICE;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }

    if (mask & EPBM_DOWN) {

      // Go to table view

      ViewType vt = VT_TABLE2;

      int i = viewData_->currentInstrumentID_;
      InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
      I_Instrument *instr = bank->GetInstrument(i);
      int table = instr->GetTable();
      if (table != VAR_OFF) {
        viewData_->currentTable_ = table;
      }
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }

    if (mask & EPBM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true,
                            viewData_->chainRow_);
    }
  } else {
    // No modifier
    if (mask & EPBM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, false,
                            viewData_->chainRow_);
    }
  }

  UIField *focus = GetFocus();
  lastFocusWasSliceGraph_ = embeddedSlicer_.HasGraphFocus(focus);
  UIIntVarField *field =
      lastFocusWasSliceGraph_ ? nullptr : (UIIntVarField *)focus;
  if (field) {
    lastFocusID_ = field->GetVariableID();
  }
};

void InstrumentView::DrawView() {
  Clear();

  GUITextProperties props;

  // Draw title
  etl::string<23> title("Instrument ");
  etl::format_spec format;
  format.width(2).fill('0').hex().upper_case(true);
  etl::to_string(viewData_->currentInstrumentID_, title, format, true);
  setTitle(title);
  // Draw fields
  if (HasModalView()) {
    if (GetFocus() == 0 && !fieldList_.empty()) {
      SetFocus(*fieldList_.begin());
    }
    for (auto it = fieldList_.begin(); it != fieldList_.end(); ++it) {
      if (embeddedSlicer_.HasGraphFocus(*it)) {
        continue;
      }
      (*it)->Draw(w_);
    }
  } else {
    FieldView::Redraw();
    drawEmbeddedSlicerWaveform();
  }
  drawMap();

  I_Instrument *instr = getInstrument();
  if (instr) {
    InstrumentType type = instr->GetType();
  }
}

void InstrumentView::AnimationUpdate() {
  GUITextProperties props;
  drawTitlebar(props);
  drawPowerButtonUI(props);
  embeddedSlicer_.AnimationUpdate(*this);
  if (isDirty_) {
    ((AppWindow &)w_).SetDirty();
  }
}

void InstrumentView::OnFocus() {
  Trace::Log("INSTRUMENTVIEW", "onFocus");

  // Get latest selected instrument, ensures we display the instrument that was
  // selected in the PhraseView
  int currentID = viewData_->currentInstrumentID_;
  Trace::Debug("INSTRUMENTVIEW", "Current instrument ID from ViewData: %d",
               currentID);

  // Get the current instrument based on the ViewData's currentInstrumentID_
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(currentID);

  if (instr) {
    // Update the instrument type field to match the current instrument
    InstrumentType currentType = instr->GetType();

    Trace::Debug("INSTRUMENTVIEW", "Current instrument type: %d", currentType);

    // Only update if the type has changed
    int currentTypeIndex = displayIndexFromInstrumentType(currentType);
    if (instrumentType_.GetInt() != currentTypeIndex) {
      Trace::Log("INSTRUMENTVIEW",
                 "OnFocus instrument type changed from %d to %d",
                 instrumentType_.GetInt(), currentType);
      // Set the instrument type without triggering the observer update
      // because we dont want the observer to do its normal check for a modified
      // instrument
      instrumentType_.SetInt(currentTypeIndex, false);
    }

    // Always refresh the UI fields when focusing the view in case of instrument
    // change from last time
    onInstrumentTypeChange(true);
  }
}

void InstrumentView::Update(Observable &o, I_ObservableData *data) {

  if (!hasFocus_) {
    return;
  }

  uintptr_t fourcc = (uintptr_t)data;

  switch (fourcc) {
  case FourCC::SampleInstrumentSlices:
  case FourCC::SampleInstrumentStart:
    if (isEmbeddedSlicerActive() &&
        embeddedSlicer_.HandleVariableUpdate(*this, fourcc)) {
      ((AppWindow &)w_).SetDirty();
    }
    break;
  case FourCC::SampleSourceInstrumentStart:
  case FourCC::SampleSourceInstrumentLoopStart:
  case FourCC::SampleSourceInstrumentEnd:
    if (isEmbeddedSlicerActive()) {
      uint32_t sample = 0;
      embeddedSlicer_.SetFocusSampleHint(
          focusedEmbeddedSlicerPointSample(sample), sample);
      embeddedSlicer_.SetPreviewFocusHint(focusedEmbeddedSlicerPreviewField());
      updateEmbeddedSlicerPreviewRange();
      if (embeddedSlicer_.UpdateZoomWindow()) {
        embeddedSlicer_.InvalidateWaveform();
      }
      embeddedSlicer_.Graph().RequestFullRedraw();
      isDirty_ = true;
      ((AppWindow &)w_).SetDirty();
    }
    break;
  case FourCC::SampleSourceInstrumentLoopMode:
    if (isEmbeddedSlicerActive()) {
      updateEmbeddedSlicerPreviewRange();
      embeddedSlicer_.Graph().RequestFullRedraw();
      isDirty_ = true;
      ((AppWindow &)w_).SetDirty();
    }
    if (skipSampleSourceOscillatorIfSampleTooLarge(
            getInstrument(), pendingSampleSourceLoopModeDirection_)) {
      isDirty_ = true;
      ((AppWindow &)w_).SetDirty();
    }
    break;
  case FourCC::ActionAutoSlice:
    if (isEmbeddedSlicerActive()) {
      if (embeddedSlicer_.HasSlicesForPlayback()) {
        MessageBox *mb = MessageBox::Create(*this, "Replace current slices?",
                                            MBBF_YES | MBBF_NO);
        embeddedSlicer_.MarkModalRecovery(0);
        embeddedSlicer_.ClearWaveformRegion(*this);
        ((AppWindow &)w_).InvalidateTextCache();
        DoModal(mb, ModalViewCallback::create<
                        &InstrumentView::AutoSliceConfirmCallback>());
      } else {
        embeddedSlicer_.AutoSliceEvenly(*this);
        updateSliceCountLabel(sliceCountLabel_, getInstrument());
        ((AppWindow &)w_).SetDirty();
      }
    }
    break;
  case FourCC::VarInstrumentType: {
    // Get the current instrument to determine its actual type
    I_Instrument *instr = getInstrument();
    InstrumentType currentType = instr ? instr->GetType() : IT_NONE;

    // Store the proposed instrument type BEFORE we revert the UI
    InstrumentType proposedType =
        kDisplayIndexToInstrumentType[instrumentType_.GetInt()];

    // Revert the UI field back to the current type until confirmed
    instrumentType_.SetInt(displayIndexFromInstrumentType(currentType), false);

    // Check if player is running
    Player *player = Player::GetInstance();
    if (!player->IsRunning()) {
      // Check if any instrument field has been modified
      bool instrumentModified = checkInstrumentModified();
      if (instrumentModified) {
        MessageBox *mb = MessageBox::Create(
            *this, "Change Instrument &", "lose settings?", MBBF_YES | MBBF_NO);
        pendingInstrumentType_ = proposedType;
        if (isEmbeddedSlicerActive()) {
          embeddedSlicer_.MarkModalRecovery(0);
          embeddedSlicer_.ClearWaveformRegion(*this);
        }
        DoModal(
            mb,
            ModalViewCallback::create<
                InstrumentView, &InstrumentView::onConfirmInstrumentTypeChange>(
                *this));
      } else {
        // Apply the proposed type change immediately if not modified
        instrumentType_.SetInt(displayIndexFromInstrumentType(proposedType),
                               false);
        onInstrumentTypeChange();
      }
    } else {
      MessageBox *mb = MessageBox::Create(*this, "Not while playing", MBBF_OK);
      DoModal(mb);
    }
    break;
  }
  case FourCC::ActionExport: {
    handleInstrumentExport();
  } break;
  case FourCC::ActionImport: {
    // Switch to the InstrumentImportView
    ViewType vt = VT_INSTRUMENT_IMPORT;
    ViewEvent ve(VET_SWITCH_VIEW, &vt);
    SetChanged();
    NotifyObservers(&ve);
  } break;
  case FourCC::SampleInstrumentSample:
  case FourCC::SampleSourceInstrumentSample: {
    I_Instrument *instr = getInstrument();
    if (!instr || (instr->GetType() != IT_SAMPLE &&
                   instr->GetType() != IT_SAMPLE_SOURCE)) {
      break;
    }

    int newIndex = -1;
    if (instr->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instr);
      newIndex = sampleInstr->GetSampleIndex();
    } else if (instr->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *sampleSourceInstr =
          static_cast<SampleSourceInstrument *>(instr);
      newIndex = sampleSourceInstr->GetSampleIndex();
    }

    if (suppressSampleChangeWarning_) {
      suppressSampleChangeWarning_ = false;
      lastSampleIndex_ = newIndex;
      break;
    }

    if (newIndex == lastSampleIndex_) {
      break;
    }

    bool hasSlicesForWarning = false;
    if (instr->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instr);
      hasSlicesForWarning = sampleInstr->HasSlicesForWarning();
      if (!hasSlicesForWarning) {
        sampleInstr->ClearSlices();
      }
    } else if (instr->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *sampleSourceInstr =
          static_cast<SampleSourceInstrument *>(instr);
      hasSlicesForWarning = sampleSourceInstr->HasSlicesForWarning();
      if (!hasSlicesForWarning) {
        sampleSourceInstr->ClearSlices();
      }
    }
    if (!hasSlicesForWarning) {
      resetSampleSourceOscillatorIfSampleTooLarge(instr);
      lastSampleIndex_ = newIndex;
      updateSliceCountLabel(sliceCountLabel_, instr);
      if (instr->GetType() == IT_SAMPLE_SOURCE) {
        setupEmbeddedSlicer();
      }
      isDirty_ = true;
      break;
    }

    MessageBox *mb = MessageBox::Create(*this, "Change sample &",
                                        "clear slices?", MBBF_YES | MBBF_NO);
    pendingSampleChangeInstrument_ = instr;
    pendingSampleChangeNewIndex_ = newIndex;
    if (instr->GetType() == IT_SAMPLE_SOURCE) {
      embeddedSlicer_.MarkModalRecovery(0);
      embeddedSlicer_.ClearWaveformRegion(*this);
    }
    DoModal(mb,
            ModalViewCallback::create<InstrumentView,
                                      &InstrumentView::onConfirmSampleChange>(
                *this));
  } break;
  case FourCC::ActionShowSampleSlices: {
    I_Instrument *instr = getInstrument();
    if (!instr || (instr->GetType() != IT_SAMPLE &&
                   instr->GetType() != IT_SAMPLE_SOURCE)) {
      break;
    }
    int sampleIndex = -1;
    if (instr->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instr);
      sampleIndex = sampleInstr->GetSampleIndex();
    } else if (instr->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *sampleSourceInstr =
          static_cast<SampleSourceInstrument *>(instr);
      sampleIndex = sampleSourceInstr->GetSampleIndex();
    }
    if (sampleIndex < 0) {
      MessageBox *mb =
          MessageBox::Create(*this, "Assign a sample first", MBBF_OK);
      DoModal(mb);
      break;
    }
    ViewType vt = VT_SAMPLE_SLICES;
    ViewEvent ve(VET_SWITCH_VIEW, &vt);
    SetChanged();
    NotifyObservers(&ve);
  } break;
  case FourCC::MidiInstrumentProgram: {
    // When program value changes, send a MIDI Program Change message during
    // playback
    if (!Player::GetInstance()->IsRunning()) {
      break;
    }

    I_Instrument *instr = getInstrument();
    if (instr && instr->GetType() == IT_MIDI) {
      MidiInstrument *midiInstr = (MidiInstrument *)instr;

      // Get the channel and program values
      Variable *channelVar =
          midiInstr->FindVariable(FourCC::MidiInstrumentChannel);
      Variable *programVar =
          midiInstr->FindVariable(FourCC::MidiInstrumentProgram);

      if (channelVar && programVar) {
        int channel = channelVar->GetInt();
        int program = programVar->GetInt();

        // Send Program Change message
        midiInstr->SendProgramChange(channel, program);
      }
    }
  } break;
  default:
    break;
  }
}

bool InstrumentView::checkInstrumentModified() {
  // Get current instrument
  I_Instrument *instrument = getInstrument();
  if (!instrument) {
    return false;
  }

  // Get the list of variables for this instrument
  etl::ivector<Variable *> *variables = instrument->Variables();
  if (!variables) {
    return false;
  }

  // Check if any variable has been modified from its default value
  for (auto it = variables->begin(); it != variables->end(); ++it) {
    Variable *var = *it;
    if (var && var->IsModified()) {
      return true;
    }
  }

  // No variables have been modified
  return false;
}

void InstrumentView::resetInstrumentToDefaults() {
  // Get current instrument
  I_Instrument *instrument = getInstrument();
  if (!instrument) {
    return;
  }

  // Get the list of variables for this instrument
  etl::ivector<Variable *> *variables = instrument->Variables();
  if (!variables) {
    return;
  }

  // Reset all variables to their default values
  for (auto it = variables->begin(); it != variables->end(); ++it) {
    Variable *var = *it;
    if (var) {
      var->Reset();
    }
  }
}

void InstrumentView::handleInstrumentExport() {
  // Get current instrument using its id
  I_Instrument *instrument =
      viewData_->project_->GetInstrumentBank()->GetInstrument(
          viewData_->currentInstrumentID_);

  // Check if the instrument has a name set
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> name = instrument->GetDisplayName();
  // Check if the name is empty, the default value, or matches the default
  // instrument type name
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> defaultTypeName =
      instrument->GetDefaultName();

  if (name.empty() || name == defaultTypeName) {
    // Show error message if no name is set
    MessageBox *mb = MessageBox::Create(*this, "Please set a name",
                                        "before exporting", MBBF_OK);
    DoModal(mb);
  } else {
    // Export the instrument using the name field
    PersistencyResult result =
        PersistencyService::GetInstance()->ExportInstrument(instrument, name);

    if (result == PERSIST_EXISTS) {
      // File already exists, ask user if they want to override it
      etl::string<strlen("Overwrite existing file: ")> confirmMsg =
          "Overwrite existing file?";
      MessageBox *mb = MessageBox::Create(*this, confirmMsg.c_str(),
                                          name.c_str(), MBBF_YES | MBBF_NO);

      exportInstrument_ = instrument;
      exportName_ = name;
      DoModal(
          mb,
          ModalViewCallback::create<InstrumentView,
                                    &InstrumentView::onConfirmExportOverwrite>(
              *this));
    } else {
      // Create a message with the instrument name
      etl::string<MAX_INSTRUMENT_NAME_LENGTH + strlen("Exported: ")>
          successMsg = "Exported: ";
      successMsg += name;

      const char *message = result == PERSIST_SAVED
                                ? successMsg.c_str()
                                : "Failed to export instrument";
      // Show export result message
      MessageBox *mb = MessageBox::Create(*this, message, MBBF_OK);
      DoModal(mb);
    }
  }
}

void InstrumentView::onConfirmInstrumentTypeChange(View &, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    applyProposedTypeChangeUI();
  }
  if (isEmbeddedSlicerActive()) {
    embeddedSlicer_.MarkModalRecovery(2);
  }
  isDirty_ = true;
}

void InstrumentView::onConfirmResetInstrument(View &, ModalView &dialog) {
  I_Instrument *instr = pendingPurgeInstrument_;
  pendingPurgeInstrument_ = nullptr;

  if (dialog.GetReturnCode() != MBL_YES || !instr) {
    return;
  }

  instr->Purge();
  isDirty_ = true;
}

void InstrumentView::onConfirmSampleChange(View &, ModalView &dialog) {
  I_Instrument *sampleInstr = pendingSampleChangeInstrument_;
  int newIndex = pendingSampleChangeNewIndex_;
  pendingSampleChangeInstrument_ = nullptr;
  pendingSampleChangeNewIndex_ = -1;

  if (!sampleInstr) {
    return;
  }

  if (dialog.GetReturnCode() == MBL_YES) {
    if (sampleInstr->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstrument =
          static_cast<SampleInstrument *>(sampleInstr);
      sampleInstrument->ClearSlices();
    } else if (sampleInstr->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *sampleSourceInstrument =
          static_cast<SampleSourceInstrument *>(sampleInstr);
      sampleSourceInstrument->ClearSlices();
    }
    lastSampleIndex_ = newIndex;
    resetSampleSourceOscillatorIfSampleTooLarge(sampleInstr);
    updateSliceCountLabel(sliceCountLabel_, sampleInstr);
    if (sampleInstr->GetType() == IT_SAMPLE_SOURCE) {
      setupEmbeddedSlicer();
      embeddedSlicer_.MarkModalRecovery(2);
    }
    isDirty_ = true;
    return;
  }

  suppressSampleChangeWarning_ = true;
  Variable *sampleVar = nullptr;
  if (sampleInstr->GetType() == IT_SAMPLE) {
    SampleInstrument *sampleInstrument =
        static_cast<SampleInstrument *>(sampleInstr);
    sampleVar = sampleInstrument->FindVariable(FourCC::SampleInstrumentSample);
  } else if (sampleInstr->GetType() == IT_SAMPLE_SOURCE) {
    SampleSourceInstrument *sampleSourceInstrument =
        static_cast<SampleSourceInstrument *>(sampleInstr);
    sampleVar = sampleSourceInstrument->FindVariable(
        FourCC::SampleSourceInstrumentSample);
  }
  if (sampleVar) {
    sampleVar->SetInt(lastSampleIndex_);
  }
  if (sampleInstr->GetType() == IT_SAMPLE_SOURCE) {
    setupEmbeddedSlicer();
    embeddedSlicer_.MarkModalRecovery(2);
  }
  isDirty_ = true;
}

void InstrumentView::onConfirmExportOverwrite(View &, ModalView &dialog) {
  I_Instrument *instrument = exportInstrument_;
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> name = exportName_;
  exportInstrument_ = nullptr;
  exportName_.clear();

  if (dialog.GetReturnCode() != MBL_YES || !instrument) {
    return;
  }

  PersistencyService::GetInstance()->ExportInstrument(instrument, name, true);
  Trace::Log("INSTRUMENTVIEW", "Instrument '%s' exported with overwrite",
             name.c_str());
}
