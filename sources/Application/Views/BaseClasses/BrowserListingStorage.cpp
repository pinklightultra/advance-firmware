/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "BrowserListingStorage.h"
#include <new>

namespace {

#define BROWSER_STORAGE_ATTRIBUTES                                             \
  __attribute__((section(".RAM_EXT"))) __attribute__((aligned(32)))

alignas(BrowserFileEntries) static unsigned char browserFileEntriesBuffer
    [sizeof(BrowserFileEntries)] BROWSER_STORAGE_ATTRIBUTES;

BrowserFileEntries &GetSharedBrowserFileEntries() {
  static BrowserFileEntries *entries = nullptr;
  if (entries == nullptr) {
    entries = new (browserFileEntriesBuffer) BrowserFileEntries();
  }
  return *entries;
}

#undef BROWSER_STORAGE_ATTRIBUTES

} // namespace

BrowserFileEntries &GetImportBrowserFileEntries() {
  return GetSharedBrowserFileEntries();
}

BrowserFileEntries &GetThemeBrowserFileEntries() {
  return GetSharedBrowserFileEntries();
}

BrowserFileEntries &GetInstrumentBrowserFileEntries() {
  return GetSharedBrowserFileEntries();
}

BrowserFileEntries &GetProjectBrowserFileEntries() {
  return GetSharedBrowserFileEntries();
}
