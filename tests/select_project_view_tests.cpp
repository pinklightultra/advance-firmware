/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Persistency/PersistencyService.h"
#include "boundary_test_utils.h"
#include "doctest/doctest.h"
#include "System/FileSystem/FakeFileSystem.h"
#include "System/FileSystem/PathUtils.h"

// Helper that mirrors the filtering logic in SelectProjectView::setCurrentFolder()
// so we can unit-test it independently of the UI framework.
static bool isValidAdvanceProject(const char *projectName, FileSystem *fs) {
  using PathString = FileSystemPath::PathString;
  PathString projectPath;
  if (!FileSystemPath::JoinPath(PROJECTS_DIR, projectName, projectPath)) {
    return false;
  }
  PathString savePath;
  if (!FileSystemPath::JoinPath(projectPath.c_str(), PROJECT_DATA_FILE,
                                savePath)) {
    return false;
  }
  return fs->exists(savePath.c_str());
}

TEST_CASE("SelectProjectView: valid Advance project with ptsav.dat is accepted") {
  BoundaryTestEnvironment env;
  auto fs = FileSystem::GetInstance();

  env.fs.AddDirectory("/projects/mySong");
  env.fs.AddDirectory("/projects/mySong/samples");
  env.fs.AddFile("/projects/mySong/ptsav.dat", "project data");
  env.fs.AddFile("/projects/mySong/samples/kick.wav", "kick sample");

  CHECK(isValidAdvanceProject("mySong", fs));
}

TEST_CASE(
    "SelectProjectView: pico project without ptsav.dat is rejected") {
  BoundaryTestEnvironment env;
  auto fs = FileSystem::GetInstance();

  // A pico-style project: has samples but no ptsav.dat
  env.fs.AddDirectory("/projects/picoSong");
  env.fs.AddDirectory("/projects/picoSong/samples");
  env.fs.AddFile("/projects/picoSong/samples/snare.wav", "snare sample");
  env.fs.AddFile("/projects/picoSong/track1.lgt", "pico track data");

  CHECK_FALSE(isValidAdvanceProject("picoSong", fs));
}

TEST_CASE(
    "SelectProjectView: empty directory without ptsav.dat is rejected") {
  BoundaryTestEnvironment env;
  auto fs = FileSystem::GetInstance();

  // A bare directory - not a valid project at all
  env.fs.AddDirectory("/projects/emptyDir");

  CHECK_FALSE(isValidAdvanceProject("emptyDir", fs));
}

TEST_CASE(
    "SelectProjectView: non-existent project directory is rejected") {
  BoundaryTestEnvironment env;
  auto fs = FileSystem::GetInstance();

  // Directory doesn't exist at all
  CHECK_FALSE(isValidAdvanceProject("doesNotExist", fs));
}

TEST_CASE("SelectProjectView: mixed list filters correctly") {
  BoundaryTestEnvironment env;
  auto fs = FileSystem::GetInstance();

  // Set up a mix of valid and invalid projects
  env.fs.AddDirectory("/projects/validAdvance");
  env.fs.AddFile("/projects/validAdvance/ptsav.dat", "state");

  env.fs.AddDirectory("/projects/picoOnly");
  env.fs.AddFile("/projects/picoOnly/song.lgt", "pico data");

  env.fs.AddDirectory("/projects/anotherValid");
  env.fs.AddDirectory("/projects/anotherValid/samples");
  env.fs.AddFile("/projects/anotherValid/ptsav.dat", "state2");
  env.fs.AddFile("/projects/anotherValid/samples/bass.wav", "bass");

  env.fs.AddDirectory("/projects/nothingHere");

  CHECK(isValidAdvanceProject("validAdvance", fs));
  CHECK_FALSE(isValidAdvanceProject("picoOnly", fs));
  CHECK(isValidAdvanceProject("anotherValid", fs));
  CHECK_FALSE(isValidAdvanceProject("nothingHere", fs));
  CHECK_FALSE(isValidAdvanceProject("nonExistent", fs));
}
