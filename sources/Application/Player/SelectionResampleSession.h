/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SELECTION_RESAMPLE_SESSION_H_
#define _SELECTION_RESAMPLE_SESSION_H_

#include "SelectionResampleRequest.h"
#include <stdint.h>

enum class SelectionResampleFinalizeStatus : uint8_t {
  None,
  Completed,
  Cancelled,
  Failed,
};

namespace SelectionResampleSession {

bool Begin(const SelectionResampleRequest &request);
void MarkCanceled();
SelectionResampleFinalizeStatus FinalizeIfStopped();
bool GetActiveRequest(SelectionResampleRequest &request);
bool IsActive();
bool IsCanceled();
const char *GetLastError();

} // namespace SelectionResampleSession

#endif
