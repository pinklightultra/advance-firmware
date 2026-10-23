/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "System/FileSystem/PathUtils.h"
#include "doctest/doctest.h"

namespace {

const char *normalize(const char *path) {
  static FileSystemPath::PathString normalized;
  normalized.clear();
  CHECK(FileSystemPath::NormalizePath(path, normalized));
  return normalized.c_str();
}

} // namespace

TEST_CASE("Path utils detect absolute paths") {
  CHECK(FileSystemPath::IsAbsolutePath("/"));
  CHECK(FileSystemPath::IsAbsolutePath("/samples/kick.wav"));
  CHECK_FALSE(FileSystemPath::IsAbsolutePath("samples/kick.wav"));
  CHECK_FALSE(FileSystemPath::IsAbsolutePath(""));
  CHECK_FALSE(FileSystemPath::IsAbsolutePath(nullptr));
}

TEST_CASE("Path utils normalize dot segments and repeated separators") {
  CHECK(std::string(normalize("/samples/./drums//kick.wav")) ==
        "/samples/drums/kick.wav");
  CHECK(std::string(normalize("/samples/library/../kick.wav")) ==
        "/samples/kick.wav");
  CHECK(std::string(normalize("samples/one/../two.wav")) == "samples/two.wav");
}

TEST_CASE("Path utils reject traversal that escapes the known root") {
  FileSystemPath::PathString normalized;
  CHECK_FALSE(FileSystemPath::NormalizePath("/../../escape.wav", normalized));
  CHECK_FALSE(FileSystemPath::NormalizePath("../escape.wav", normalized));
}

TEST_CASE("Path utils join explicit base directories with relative leaves") {
  FileSystemPath::PathString joined;
  CHECK(FileSystemPath::JoinPath("/samples/library", "../kick.wav", joined));
  CHECK(joined == "/samples/kick.wav");

  CHECK_FALSE(FileSystemPath::JoinPath("/samples", "/kick.wav", joined));
  CHECK_FALSE(FileSystemPath::JoinPath("samples", "kick.wav", joined));
}

TEST_CASE("Path utils compute dirname and basename from normalized paths") {
  FileSystemPath::PathString dirName;
  FileSystemPath::PathString baseName;

  CHECK(FileSystemPath::DirName("/samples/kick.wav", dirName));
  CHECK(dirName == "/samples");

  CHECK(FileSystemPath::DirName("/kick.wav", dirName));
  CHECK(dirName == "/");

  CHECK(FileSystemPath::BaseName("/samples/kick.wav", baseName));
  CHECK(baseName == "kick.wav");

  CHECK(FileSystemPath::BaseName("/", baseName));
  CHECK(baseName == "/");
}

TEST_CASE("Path utils detect parent-child relationships explicitly") {
  CHECK(FileSystemPath::IsParentOf("/samples", "/samples/kick.wav"));
  CHECK(FileSystemPath::IsParentOf("/", "/samples/kick.wav"));
  CHECK_FALSE(FileSystemPath::IsParentOf("/samples", "/samples"));
  CHECK_FALSE(FileSystemPath::IsParentOf("/samples", "/projects/demo.pt"));
}
