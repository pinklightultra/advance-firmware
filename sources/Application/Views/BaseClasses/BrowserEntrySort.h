/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _BROWSER_ENTRY_SORT_H_
#define _BROWSER_ENTRY_SORT_H_

#include "Externals/etl/include/etl/algorithm.h"
#include "Externals/etl/include/etl/string_utilities.h"
#include "System/FileSystem/FileSystem.h"

inline void SortBrowserEntries(etl::ivector<FileSystemEntry> &entries) {
  etl::sort(entries.begin(), entries.end(),
            [](const FileSystemEntry &lhs, const FileSystemEntry &rhs) {
              auto lhsName = lhs.name;
              auto rhsName = rhs.name;
              etl::to_lower_case(lhsName);
              etl::to_lower_case(rhsName);
              return lhsName < rhsName;
            });
}

#endif // _BROWSER_ENTRY_SORT_H_
