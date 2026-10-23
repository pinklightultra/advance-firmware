/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Views/BaseClasses/BrowserEntrySort.h"
#include "doctest/doctest.h"

namespace {

etl::string<PFILENAME_SIZE> ToLowerCopy(const etl::string<PFILENAME_SIZE> &name) {
  auto lowerName = name;
  etl::to_lower_case(lowerName);
  return lowerName;
}

} // namespace

TEST_CASE("SortBrowserEntries sorts filenames case-insensitively") {
  etl::vector<FileSystemEntry, MAX_FILE_INDEX_SIZE> entries;

  for (const char *name :
       {"zeta.wav", "Alpha.wav", "beta.wav", "alpha.wav", "Aardvark.wav"}) {
    FileSystemEntry entry;
    entry.name = name;
    entries.push_back(entry);
  }

  SortBrowserEntries(entries);

  REQUIRE(entries.size() == 5);
  CHECK(entries[0].name == "Aardvark.wav");
  CHECK(ToLowerCopy(entries[1].name) == "alpha.wav");
  CHECK(ToLowerCopy(entries[2].name) == "alpha.wav");
  CHECK(entries[3].name == "beta.wav");
  CHECK(entries[4].name == "zeta.wav");
}

TEST_CASE("SortBrowserEntries keeps parent entry first") {
  etl::vector<FileSystemEntry, MAX_FILE_INDEX_SIZE> entries;

  for (const char *name : {"zeta.wav", "..", "Alpha.wav"}) {
    FileSystemEntry entry;
    entry.name = name;
    entries.push_back(entry);
  }

  SortBrowserEntries(entries);

  REQUIRE(entries.size() == 3);
  CHECK(entries[0].name == "..");
  CHECK(entries[1].name == "Alpha.wav");
  CHECK(entries[2].name == "zeta.wav");
}

TEST_CASE("SortBrowserEntries treats equal lowercase names as equivalent") {
  etl::vector<FileSystemEntry, MAX_FILE_INDEX_SIZE> entries;

  for (const char *name : {"alpha.wav", "Alpha.wav", "ALPHA.wav"}) {
    FileSystemEntry entry;
    entry.name = name;
    entries.push_back(entry);
  }

  SortBrowserEntries(entries);

  REQUIRE(entries.size() == 3);
  CHECK(ToLowerCopy(entries[0].name) == "alpha.wav");
  CHECK(ToLowerCopy(entries[1].name) == "alpha.wav");
  CHECK(ToLowerCopy(entries[2].name) == "alpha.wav");
}
