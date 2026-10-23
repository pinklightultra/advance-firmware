/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef SAMPLE_EDIT_PROGRESS_DISPLAY_H_
#define SAMPLE_EDIT_PROGRESS_DISPLAY_H_

#include "Application/Persistency/PersistenceConstants.h"
#include "Externals/etl/include/etl/string.h"
#include "System/FileSystem/FileSystem.h"

class SampleEditProgressDisplay {
public:
  explicit SampleEditProgressDisplay(const etl::string<PFILENAME_SIZE> &path);

  void Update(uint8_t percent);
  void Finish(bool success);

private:
  static constexpr char spinnerChars_[4] = {'|', '/', '-', '\\'};
  etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> sampleName_;
  uint8_t spinnerIndex_;
};

#endif // SAMPLE_EDIT_PROGRESS_DISPLAY_H_
