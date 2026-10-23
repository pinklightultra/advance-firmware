/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Persistency/PersistencyService.h"
#include "Foundation/Variables/StringVariable.h"
#include "boundary_test_utils.h"
#include "doctest/doctest.h"
#include <string>

namespace {

class TestInstrument : public I_Instrument {
public:
  TestInstrument() : name_(FourCC::InstrumentName, "") {
    variables_.push_back(&name_);
  }

  InstrumentType GetType() override { return IT_SAMPLE; }

  bool Restore(PersistencyDocument *doc) override {
    restoreCalled_ = (doc != nullptr);
    restoredElementName_ =
        (doc != nullptr && doc->ElemName() != nullptr) ? doc->ElemName() : "";
    return restoreResult_;
  }

  void SetRestoreResult(bool restoreResult) { restoreResult_ = restoreResult; }
  bool restoreCalled() const { return restoreCalled_; }
  const char *restoredElementName() const { return restoredElementName_; }
  std::string instrumentName() { return name_.GetString().c_str(); }

private:
  StringVariable<MAX_INSTRUMENT_NAME_LENGTH> name_;
  bool restoreCalled_ = false;
  bool restoreResult_ = true;
  const char *restoredElementName_ = "";
};

} // namespace

TEST_CASE(
    "PersistencyService detects instrument type from an absolute path without "
    "cwd changes") {
  BoundaryTestEnvironment env;
  env.fs.AddDirectory("/imports");
  env.fs.AddFile("/imports/Bass.pti",
                 "<INSTRUMENT TYPE=\"SAMPLE\" VERSION=\"1\"></INSTRUMENT>");
  PersistencyService *persistency = PersistencyService::GetInstance();

  env.fs.ClearOperationLog();
  const InstrumentType type =
      persistency->DetectInstrumentType("/imports/Bass.pti");

  CHECK(type == IT_SAMPLE);
  CHECK(operationLogHasNoChdir(env.fs));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type == FakeFileSystemOperationType::Open &&
                             entry.resolvedPath == "/imports/Bass.pti" &&
                             entry.success;
                    }));
}

TEST_CASE(
    "PersistencyService imports instruments from an absolute path and sets "
    "the name from the leaf filename") {
  BoundaryTestEnvironment env;
  env.fs.AddDirectory("/imports");
  env.fs.AddFile("/imports/Bass Lead.pti",
                 "<INSTRUMENT TYPE=\"SAMPLE\" VERSION=\"1\"></INSTRUMENT>");
  PersistencyService *persistency = PersistencyService::GetInstance();
  TestInstrument instrument;

  env.fs.ClearOperationLog();
  const PersistencyResult result =
      persistency->ImportInstrument(&instrument, "/imports/Bass Lead.pti");

  REQUIRE(result == PERSIST_LOADED);
  CHECK(instrument.restoreCalled());
  CHECK(instrument.instrumentName() == "Bass Lead");
  CHECK(operationLogHasNoChdir(env.fs));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type == FakeFileSystemOperationType::Open &&
                             entry.resolvedPath == "/imports/Bass Lead.pti" &&
                             entry.success;
                    }));
}

TEST_CASE("PersistencyService still resolves legacy relative instrument names "
          "through the instruments dir") {
  BoundaryTestEnvironment env;
  env.fs.AddDirectory("/instruments");
  env.fs.AddFile("/instruments/Pad.pti",
                 "<INSTRUMENT TYPE=\"SAMPLE\"></INSTRUMENT>");
  PersistencyService *persistency = PersistencyService::GetInstance();
  TestInstrument instrument;

  env.fs.ClearOperationLog();
  const PersistencyResult result =
      persistency->ImportInstrument(&instrument, "Pad.pti");

  REQUIRE(result == PERSIST_LOADED);
  CHECK(operationLogHasNoChdir(env.fs));
  CHECK(std::any_of(env.fs.operationLog().begin(), env.fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type == FakeFileSystemOperationType::Open &&
                             entry.resolvedPath == "/instruments/Pad.pti" &&
                             entry.success;
                    }));
}
