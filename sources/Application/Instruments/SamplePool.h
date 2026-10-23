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

#ifndef _SAMPLE_POOL_H_
#define _SAMPLE_POOL_H_

#include "Application/Model/Song.h"
#include "Application/Persistency/PersistencyService.h"
#include "Foundation/Observable.h"
#include "Foundation/T_Singleton.h"
#include "WavFile.h"
#include <cstdint>

#define MAX_SAMPLES MAX_SAMPLEINSTRUMENT_COUNT * 4

enum SamplePoolEventType { SPET_INSERT, SPET_DELETE, SPET_RENAME };

struct SamplePoolEvent : public I_ObservableData {
  SamplePoolEventType type_;
  int index_;
};

class SamplePool : public T_Factory<SamplePool>, public Observable {
public:
  void Load(const char *projectName);
  SamplePool();
  virtual void Reset() = 0;
  virtual ~SamplePool();
  SoundSource *GetSource(uint32_t i);
  char **GetNameList();
  int GetNameListSize();
  uint32_t FindSampleIndexByName(
      const etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> &name);
  bool RenameLoadedSample(
      const etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> &oldName,
      const etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> &newName);
  // Import a sample into the project, then append it to the pool only after
  // the backing sample data has been fully loaded. Returns the new sample
  // index or -1 on error.
  int ImportSample(const char *name, const char *projectName,
                   int targetChannelCount);

  int LoadProjectSample(const char *path);
  void PurgeSample(int i, const char *projectName);
  virtual bool CheckSampleFits(int sampleSize) = 0;
  virtual uint32_t GetAvailableSampleStorageSpace() = 0;
  virtual bool unloadSample(uint32_t i) = 0;
  int8_t ReloadSample(uint8_t index, const char *name);

protected:
  virtual void updateStatus(uint32_t current, uint32_t total,
                            const char *message);
  virtual bool loadSample(const char *name) = 0;
  bool loadSoundFont(const char *path);
  uint32_t count_;
  char nameStore_[MAX_SAMPLES][MAX_INSTRUMENT_FILENAME_LENGTH + 1];
  char *names_[MAX_SAMPLES];
  WavFile wav_[MAX_SAMPLES];
  void swapEntries(int src, int dst);

  uint32_t importCount;
  uint32_t importIndex;
  const char *importName;

private:
  etl::vector<I_Observer *, MAX_SAMPLEINSTRUMENT_COUNT> observers_;
};

#endif
