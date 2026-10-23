/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Views/BaseClasses/BrowserPathUtils.h"
#include "doctest/doctest.h"

TEST_CASE("Browser path helpers build the active project's samples directory") {
  FileSystemPath::PathString samplesDir;
  REQUIRE(BrowserPathUtils::BuildProjectSamplesDirectory("Demo", samplesDir));
  CHECK(samplesDir == "/projects/Demo/samples");
}

TEST_CASE("Browser path helpers resolve parents within a bounded root") {
  FileSystemPath::PathString parentDir;

  REQUIRE(BrowserPathUtils::ResolveParentDirectory("/themes/packs/neon",
                                                   "/themes", parentDir));
  CHECK(parentDir == "/themes/packs");

  REQUIRE(BrowserPathUtils::ResolveParentDirectory("/samples/drums", "/",
                                                   parentDir));
  CHECK(parentDir == "/samples");

  CHECK_FALSE(BrowserPathUtils::ResolveParentDirectory("/themes", "/themes",
                                                       parentDir));
  CHECK_FALSE(BrowserPathUtils::ResolveParentDirectory("/outside", "/themes",
                                                       parentDir));
}

TEST_CASE(
    "Browser path helpers block navigating into the current project dir") {
  CHECK(BrowserPathUtils::IsCurrentProjectImportDirectory("/projects/Demo",
                                                          "Demo"));
  CHECK_FALSE(BrowserPathUtils::IsCurrentProjectImportDirectory(
      "/projects/Demo/samples", "Demo"));
  CHECK_FALSE(BrowserPathUtils::IsCurrentProjectImportDirectory(
      "/projects/Other", "Demo"));
}
