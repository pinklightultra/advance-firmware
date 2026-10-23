/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Persistency/PersistencyService.h"
#include "boundary_test_utils.h"
#include "doctest/doctest.h"

TEST_CASE("PersistencyService deletes projects using explicit absolute paths") {
  BoundaryTestEnvironment env;
  PersistencyService *persistency = PersistencyService::GetInstance();
  env.fs.AddDirectory("/projects");
  env.fs.AddDirectory("/projects/demo");
  env.fs.AddDirectory("/projects/demo/samples");
  env.fs.AddDirectory("/projects/demo/takes");
  env.fs.AddFile("/projects/demo/ptsav.dat", "state");
  env.fs.AddFile("/projects/demo/samples/kick.wav", "kick");
  env.fs.AddFile("/projects/demo/takes/clip.wav", "clip");

  env.fs.ClearOperationLog();
  const bool deleted = persistency->DeleteProject("demo");

  REQUIRE(deleted);
  CHECK_FALSE(env.fs.exists("/projects/demo"));
  CHECK_FALSE(env.fs.exists("/projects/demo/samples/kick.wav"));
  CHECK(operationLogHasNoChdir(env.fs));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::ListDirectory &&
                             entry.resolvedPath == "/projects/demo" &&
                             entry.success;
                    }));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::DeleteDir &&
                             entry.resolvedPath == "/projects/demo" &&
                             entry.success;
                    }));
}

TEST_CASE("PersistencyService deletes sibling entries after nested directory "
          "recursion") {
  BoundaryTestEnvironment env;
  PersistencyService *persistency = PersistencyService::GetInstance();
  env.fs.AddDirectory("/projects");
  env.fs.AddDirectory("/projects/demo");
  env.fs.AddDirectory("/projects/demo/aaa-nested");
  env.fs.AddDirectory("/projects/demo/zzz-dir");
  env.fs.AddFile("/projects/demo/aaa-nested/inner-a.wav", "a");
  env.fs.AddFile("/projects/demo/aaa-nested/inner-b.wav", "b");
  env.fs.AddFile("/projects/demo/aaa-nested/inner-c.wav", "c");
  env.fs.AddFile("/projects/demo/zzz-dir/clip.wav", "clip");
  env.fs.AddFile("/projects/demo/zzz-last.wav", "tail");

  env.fs.ClearOperationLog();
  const bool deleted = persistency->DeleteProject("demo");

  // Regression coverage: the old implementation iterated fileEntries_ while a
  // recursive delete repopulated that same buffer. After deleting
  // /projects/demo/aaa-nested, the stale outer iterator advances into
  // inner-b.wav and tries to delete /projects/demo/inner-b.wav, which fails.
  REQUIRE(deleted);
  CHECK_FALSE(env.fs.exists("/projects/demo"));
  CHECK_FALSE(env.fs.exists("/projects/demo/aaa-nested/inner-a.wav"));
  CHECK_FALSE(env.fs.exists("/projects/demo/aaa-nested/inner-b.wav"));
  CHECK_FALSE(env.fs.exists("/projects/demo/aaa-nested/inner-c.wav"));
  CHECK_FALSE(env.fs.exists("/projects/demo/zzz-dir/clip.wav"));
  CHECK_FALSE(env.fs.exists("/projects/demo/zzz-last.wav"));
  CHECK(operationLogHasNoChdir(env.fs));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::DeleteDir &&
                             entry.resolvedPath == "/projects/demo/zzz-dir" &&
                             entry.success;
                    }));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::DeleteFile &&
                             entry.resolvedPath ==
                                 "/projects/demo/zzz-last.wav" &&
                             entry.success;
                    }));
}

TEST_CASE("PersistencyService replaces an invalid unnamed project") {
  BoundaryTestEnvironment env;
  PersistencyService *persistency = PersistencyService::GetInstance();
  env.fs.AddDirectory("/projects");
  env.fs.AddDirectory("/projects/.untitled");
  env.fs.AddDirectory("/projects/.untitled/samples");
  env.fs.AddFile("/projects/.untitled/ptsav.dat", "invalid project data");
  env.fs.AddFile("/projects/.untitled/" AUTO_SAVE_FILENAME,
                 "invalid autosave data");
  env.fs.AddFile("/projects/.untitled/samples/stale.wav", "sample data");

  const PersistencyResult result = persistency->CreateProject();

  REQUIRE(result == PERSIST_SAVED);
  CHECK(persistency->Load(UNNAMED_PROJECT_NAME) == PERSIST_LOADED);
  CHECK(env.fs.exists("/projects/.untitled/samples"));
  CHECK_FALSE(env.fs.exists("/projects/.untitled/" AUTO_SAVE_FILENAME));
  CHECK_FALSE(env.fs.exists("/projects/.untitled/samples/stale.wav"));
}

TEST_CASE("PersistencyService save-as copies project samples from explicit "
          "absolute paths") {
  BoundaryTestEnvironment env;
  PersistencyService *persistency = PersistencyService::GetInstance();
  env.fs.AddDirectory("/projects");
  env.fs.AddDirectory("/projects/source");
  env.fs.AddDirectory("/projects/source/samples");
  env.fs.AddFile("/projects/source/samples/kick.wav", "kick");
  env.fs.AddFile("/projects/source/samples/notes.txt", "notes");
  env.fs.AddFile("/projects/source/ptsav.dat", "<PICOTRACKER/>");

  env.fs.ClearOperationLog();
  const PersistencyResult result = persistency->Save("copy", "source", true);

  REQUIRE(result == PERSIST_SAVED);
  CHECK(env.fs.exists("/projects/copy"));
  CHECK(env.fs.exists("/projects/copy/samples/kick.wav"));
  CHECK_FALSE(env.fs.exists("/projects/copy/samples/notes.txt"));
  CHECK(operationLogHasNoChdir(env.fs));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::ListDirectory &&
                             entry.resolvedPath == "/projects/source/samples" &&
                             entry.success;
                    }));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::CopyFile &&
                             entry.resolvedPath ==
                                 "/projects/source/samples/kick.wav -> "
                                 "/projects/copy/samples/kick.wav" &&
                             entry.success;
                    }));
}
