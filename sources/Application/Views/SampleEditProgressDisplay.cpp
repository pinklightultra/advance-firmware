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

#include "SampleEditProgressDisplay.h"

#include "System/FileSystem/PathUtils.h"
#include "System/io/Status.h"

SampleEditProgressDisplay::SampleEditProgressDisplay(
    const etl::string<PFILENAME_SIZE> &path)
    : spinnerIndex_(0) {
  FileSystemPath::PathString baseName;
  if (FileSystemPath::BaseName(path.c_str(), baseName)) {
    sampleName_ = baseName.c_str();
  } else {
    sampleName_ = path.c_str();
  }
}

void SampleEditProgressDisplay::Update(uint8_t percent) {
  const char spinner = spinnerChars_[spinnerIndex_++ & 0x03];
  Status::SetMultiLine("Sample edit\n%s\n%3u%% %c", sampleName_.c_str(),
                       static_cast<unsigned int>(percent), spinner);
}

void SampleEditProgressDisplay::Finish(bool success) {
  const char *result = success ? "Complete" : "Failed";
  Status::SetMultiLine("Sample edit %s\n%s", result, sampleName_.c_str());
}
