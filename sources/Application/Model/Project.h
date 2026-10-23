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

#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "Application/Instruments/InstrumentBank.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Persistency/Persistent.h"
#include "Foundation/Observable.h"
#include "Foundation/Types/Types.h"
#include "Foundation/Variables/StringVariable.h"
#include "Foundation/Variables/VariableContainer.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "Song.h"

#define MAX_TAP 3

const unsigned short MAX_TEMPO = 300;
const unsigned short MIN_TEMPO = 60;
const unsigned short DEFAULT_TEMPO = 138;

enum ProjectAudioMode {
  kProjectAudioModeStereo = 0,
  kProjectAudioModeMono = 1,
};

class Project : public Persistent, public VariableContainer, I_Observer {
public:
  Project(const char *name);
  ~Project();
  void Load(const char *name);
  void Purge();
  void PurgeInstruments();
  void PurgeSamples();

  Song song_;

  int GetMasterVolume();
  int GetChannelVolume(int channel);
  int GetChannelFXSend(int channel, int fx);
  int GetFXReturnVolume(int fx);
  int GetFXSlotType(int fx);
  int GetReverbInputGain();
  int GetReverbTime();
  int GetReverbDiffusion();
  int GetReverbLowPass();
  bool IsMixRoutingDirty() const;
  void ClearMixRoutingDirty();
  bool Wrap();
  void OnTempoTap();
  void NudgeTempo(int value);
  int GetScale();
  uint8_t GetScaleRoot();
  int GetTempo(); // Takes nudging into account
  int GetTranspose();
  int GetAudioMode();
  void GetProjectName(char *name);
  void SetProjectName(char *name);
  int32_t SampleInUse(etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename);

  void Trigger();

  // I_Observer
  virtual void Update(Observable &o, I_ObservableData *d);

  InstrumentBank *GetInstrumentBank();

  // Persistent
  virtual void SaveContent(tinyxml2::XMLPrinter *printer);
  virtual void RestoreContent(PersistencyDocument *doc);

private:
  etl::vector<Variable *, 56> variables_;

  InstrumentBank instrumentBank_;
  int tempoNudge_;
  unsigned long lastTap_[MAX_TAP];
  unsigned int tempoTapCount_;
  char *name[MAX_PROJECT_NAME_LENGTH];

  // variables
  WatchedVariable tempo_;
  WatchedVariable masterVolume_;
  // Individual channel volume variables instead of using an array
  // as initialization of such a large array causes in constructors causes stack
  // overflow issues
  WatchedVariable channelVolume1_;
  WatchedVariable channelVolume2_;
  WatchedVariable channelVolume3_;
  WatchedVariable channelVolume4_;
  WatchedVariable channelVolume5_;
  WatchedVariable channelVolume6_;
  WatchedVariable channelVolume7_;
  WatchedVariable channelVolume8_;
  WatchedVariable channel1FX1Send_;
  WatchedVariable channel2FX1Send_;
  WatchedVariable channel3FX1Send_;
  WatchedVariable channel4FX1Send_;
  WatchedVariable channel5FX1Send_;
  WatchedVariable channel6FX1Send_;
  WatchedVariable channel7FX1Send_;
  WatchedVariable channel8FX1Send_;
  WatchedVariable channel1FX2Send_;
  WatchedVariable channel2FX2Send_;
  WatchedVariable channel3FX2Send_;
  WatchedVariable channel4FX2Send_;
  WatchedVariable channel5FX2Send_;
  WatchedVariable channel6FX2Send_;
  WatchedVariable channel7FX2Send_;
  WatchedVariable channel8FX2Send_;
  WatchedVariable channel1FX3Send_;
  WatchedVariable channel2FX3Send_;
  WatchedVariable channel3FX3Send_;
  WatchedVariable channel4FX3Send_;
  WatchedVariable channel5FX3Send_;
  WatchedVariable channel6FX3Send_;
  WatchedVariable channel7FX3Send_;
  WatchedVariable channel8FX3Send_;
  WatchedVariable fx1Volume_;
  WatchedVariable fx2Volume_;
  WatchedVariable fx3Volume_;
  WatchedVariable fx1Type_;
  WatchedVariable fx2Type_;
  WatchedVariable fx3Type_;
  WatchedVariable reverbInputGain_;
  WatchedVariable reverbTime_;
  WatchedVariable reverbDiffusion_;
  WatchedVariable reverbLowPass_;
  Variable wrap_;
  Variable transpose_;
  Variable scale_;
  Variable scaleRoot_;
  StringWatchedVariable<MAX_PROJECT_NAME_LENGTH> projectName_;
  Variable projectAudioMode_;
  Variable previewVolume_;
  bool mixRoutingDirty_;

  void SyncSongChannelCount();
};

#endif
