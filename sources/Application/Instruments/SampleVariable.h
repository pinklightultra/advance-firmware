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

#ifndef _SAMPLE_VARIABLE_H_
#define _SAMPLE_VARIABLE_H_

#include "Foundation/Observable.h"
#include "Foundation/Variables/WatchedVariable.h"

class SampleVariable : public WatchedVariable, public I_Observer {
public:
  SampleVariable(FourCC id);
  ~SampleVariable();

  // True while this variable's value is being adjusted purely to track a sample
  // pool re-index (an entry below it was removed and the pool shifted the
  // remaining entries down). The underlying sample is unchanged, so observing
  // instruments must refresh their source pointer WITHOUT resetting user trim
  // points (start/loop/end).
  bool IsReindexing() const { return reindexing_; }

protected:
  virtual void Update(Observable &o, I_ObservableData *d);

private:
  bool reindexing_ = false;
};
#endif
