/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _BROWSER_LISTING_STORAGE_H_
#define _BROWSER_LISTING_STORAGE_H_

#include "Externals/etl/include/etl/vector.h"
#include "System/FileSystem/FileSystem.h"

using BrowserFileEntries = etl::vector<FileSystemEntry, MAX_FILE_INDEX_SIZE>;

BrowserFileEntries &GetImportBrowserFileEntries();
BrowserFileEntries &GetThemeBrowserFileEntries();
BrowserFileEntries &GetInstrumentBrowserFileEntries();
BrowserFileEntries &GetProjectBrowserFileEntries();

#endif
