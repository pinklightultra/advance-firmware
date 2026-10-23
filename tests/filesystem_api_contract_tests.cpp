/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "System/FileSystem/FakeFileSystem.h"
#include "doctest/doctest.h"
#include <algorithm>
#include <string>

TEST_CASE("Open rejects relative paths for the absolute-path API") {
  FakeFileSystem fs;
  fs.AddDirectory("/samples");
  fs.AddFile("/samples/kick.wav", "kick");

  auto file = fs.Open("kick.wav", "r");

  CHECK_FALSE(file);
  REQUIRE_FALSE(fs.operationLog().empty());
  CHECK(fs.operationLog().back().type == FakeFileSystemOperationType::Open);
  CHECK(fs.operationLog().back().resolvedPath.empty());
  CHECK_FALSE(fs.operationLog().back().success);
}

TEST_CASE("Legacy cwd resolution is opt-in in the host fake filesystem") {
  FakeFileSystem fs;
  fs.AddDirectory("/samples");
  fs.AddFile("/samples/snare.wav", "snare");
  REQUIRE(fs.chdir("/samples"));

  auto absoluteContract = fs.Open("snare.wav", "r");
  CHECK_FALSE(absoluteContract);

  fs.SetAllowLegacyCwdResolution(true);
  auto legacyFile = fs.Open("snare.wav", "r");

  REQUIRE(legacyFile);
  CHECK(fs.operationLog().back().resolvedPath == "/samples/snare.wav");
}

TEST_CASE("Failure injection applies to the next matching operation") {
  FakeFileSystem fs;
  fs.AddDirectory("/samples");
  fs.AddFile("/samples/kick.wav", "kick");
  fs.FailNext(FakeFileSystemOperationType::Open);

  auto file = fs.Open("/samples/kick.wav", "r");

  CHECK_FALSE(file);
  REQUIRE_FALSE(fs.operationLog().empty());
  CHECK(fs.operationLog().back().type == FakeFileSystemOperationType::Open);
  CHECK_FALSE(fs.operationLog().back().success);
}

TEST_CASE("ListDirectory uses an explicit absolute directory and returns entry "
          "metadata only") {
  FakeFileSystem fs;
  fs.AddDirectory("/samples");
  fs.AddDirectory("/samples/drums");
  fs.AddFile("/samples/kick.wav", "kick");
  fs.AddFile("/samples/readme.txt", "notes");
  fs.AddFile("/samples/.hidden.wav", "hidden");
  REQUIRE(fs.chdir("/staging") == false);
  fs.AddDirectory("/staging");
  REQUIRE(fs.chdir("/staging"));
  fs.ClearOperationLog();

  etl::vector<FileSystemEntry, MAX_FILE_INDEX_SIZE> entries;
  REQUIRE(fs.ListDirectory("/samples", &entries, ".wav", false));

  REQUIRE(entries.size() == 2);
  CHECK(entries[0].name == "drums");
  CHECK(entries[0].type == PFT_DIR);
  CHECK(entries[1].name == "kick.wav");
  CHECK(entries[1].type == PFT_FILE);
  CHECK(entries[1].size == 4);
  CHECK(std::none_of(fs.operationLog().begin(), fs.operationLog().end(),
                     [](const FakeFileSystemOperation &entry) {
                       return entry.type == FakeFileSystemOperationType::Chdir;
                     }));
  CHECK(std::any_of(fs.operationLog().begin(), fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::ListDirectory &&
                             entry.resolvedPath == "/samples" && entry.success;
                    }));
}
