/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * End-to-end tests for importing a sample instrument .pti through the real
 * PersistencyService + Persistent + SampleSourceInstrument stack.
 *
 * Reproduces issue #370 "v3: Instrument import broken": selecting a sample
 * instrument .pti for import does nothing (the imported instrument has no
 * usable sample and makes no sound).
 *
 * The TableSaveState::Reset() stub is adapted from the BSD-licensed
 * sources/Application/Player/TablePlayback.cpp.
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */
#include "doctest/doctest.h"

#define private public
#include "Application/Instruments/SamplePool.h"
#include "Application/Instruments/SampleSourceInstrument.h"
#undef private

#include "Application/Instruments/InstrumentBank.h"
#include "Application/Persistency/PersistencyService.h"
#include "System/FileSystem/FakeFileSystem.h"
#include <cstring>
#include <string>
#include <vector>

// Minimal stub so we don't have to link TablePlayback.cpp (which pulls in the
// whole player mixer graph). Mirrors TablePlayback.cpp's implementation.
void TableSaveState::Reset() {
  position_[0] = position_[1] = position_[2] = 0;
  for (int i = 0; i < TABLE_STEPS; ++i) {
    hopCount_[i][0] = hopCount_[i][1] = hopCount_[i][2] = 0;
  }
}

namespace {
class TestPool : public SamplePool {
public:
  void Reset() override { count_ = 0; }
  bool CheckSampleFits(uint32_t) override { return true; }
  uint32_t GetAvailableSampleStorageSpace() override { return 0xFFFFFFFF; }
  bool unloadSample(uint32_t) override { return false; }
  void Seed(const std::vector<std::string> &names) {
    count_ = (uint32_t)names.size();
    for (uint32_t i = 0; i < names.size(); ++i) {
      std::strncpy(nameStore_[i], names[i].c_str(),
                   MAX_INSTRUMENT_FILENAME_LENGTH);
      nameStore_[i][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
    }
  }

protected:
  bool loadSample(const char *) override { return false; }
};

struct PoolGuard {
  explicit PoolGuard(SamplePool *p) { SamplePool::Install(p); }
  ~PoolGuard() { SamplePool::Install(nullptr); }
};
struct FsGuard {
  explicit FsGuard(FileSystem *fs) { FileSystem::Install(fs); }
  ~FsGuard() { FileSystem::Install(nullptr); }
};

// A representative sample instrument export, pretty-printed exactly as
// PersistencyService::ExportInstrument writes it (tinyxml2 XMLPrinter). The
// sample is referenced by *name* only; the WAV itself is not part of the file.
const char *kSampleInstrumentPti =
    "<INSTRUMENT VERSION=\"3.0\" TYPE=\"SAMPLESOURCE\">\n"
    "    <PARAM NAME=\"InstrumentName\" VALUE=\"MySnare\"/>\n"
    "    <PARAM NAME=\"sample\" VALUE=\"Snare.wav\"/>\n"
    "    <PARAM NAME=\"root note\" VALUE=\"62\"/>\n"
    "</INSTRUMENT>\n";

I_Instrument *makeSampleSourceSlot(InstrumentBank &bank) {
  bank.GetNextAndAssignID(IT_SAMPLE_SOURCE, 0);
  return bank.GetInstrument(0);
}
} // namespace

// Characterisation test: the parameter restore path itself works fine when the
// referenced sample already exists in the target project's pool.
TEST_CASE(
    "Sample instrument import restores params when the sample is present") {
  TestPool pool;
  PoolGuard pg(&pool);
  pool.Seed({"Kick.wav", "Snare.wav"});

  FakeFileSystem fs;
  FsGuard fg(&fs);
  fs.AddDirectory("/instruments");
  fs.AddFile("/instruments/MySnare.pti", kSampleInstrumentPti);

  PersistencyService *p = PersistencyService::GetInstance();
  REQUIRE(p->DetectInstrumentType("/instruments/MySnare.pti") ==
          IT_SAMPLE_SOURCE);

  InstrumentBank bank;
  I_Instrument *inst = makeSampleSourceSlot(bank);
  REQUIRE(inst != nullptr);

  const PersistencyResult r =
      p->ImportInstrument(inst, "/instruments/MySnare.pti");

  CHECK(r == PERSIST_LOADED);
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentSample)->GetInt() ==
        1);
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentRootNote)->GetInt() ==
        62);
  CHECK_FALSE(inst->IsEmpty());
}

TEST_CASE("Importing a sample instrument makes its sample available [#370]") {
  TestPool pool;
  PoolGuard pg(&pool);
  // Fresh/other project: the referenced "Snare.wav" is NOT already loaded.
  pool.Seed({"Kick.wav", "HiHat.wav"});

  FakeFileSystem fs;
  FsGuard fg(&fs);
  fs.AddDirectory("/instruments");
  fs.AddFile("/instruments/MySnare.pti", kSampleInstrumentPti);

  PersistencyService *p = PersistencyService::GetInstance();
  InstrumentBank bank;
  I_Instrument *inst = makeSampleSourceSlot(bank);

  const PersistencyResult r =
      p->ImportInstrument(inst, "/instruments/MySnare.pti");
  REQUIRE(r == PERSIST_LOADED);

  // The imported instrument should be usable (have a resolved sample). Today it
  // is empty, so this fails.
  CHECK_FALSE(inst->IsEmpty());
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentSample)->GetInt() !=
        -1);
}

TEST_CASE("Purging a lower-indexed sample preserves instrument trim points") {
  TestPool pool;
  PoolGuard pg(&pool);
  // Instrument will point at index 2 ("Used.wav"); index 0 is the unused sample
  // that gets purged, shifting "Used.wav" down to index 1.
  pool.Seed({"Unused.wav", "Other.wav", "Used.wav"});

  FakeFileSystem fs;
  FsGuard fg(&fs);

  InstrumentBank bank;
  I_Instrument *inst = makeSampleSourceSlot(bank);
  REQUIRE(inst != nullptr);

  // Assign the sample; this resets the trim points via updateInstrumentData().
  inst->FindVariable(FourCC::SampleSourceInstrumentSample)->SetInt(2);
  REQUIRE(inst->FindVariable(FourCC::SampleSourceInstrumentSample)->GetInt() ==
          2);

  // User dials in custom trim points on the instrument.
  const int kCustomStart = 100;
  const int kCustomLoopStart = 200;
  const int kCustomEnd = 4242;
  inst->FindVariable(FourCC::SampleSourceInstrumentStart)->SetInt(kCustomStart);
  inst->FindVariable(FourCC::SampleSourceInstrumentLoopStart)
      ->SetInt(kCustomLoopStart);
  inst->FindVariable(FourCC::SampleSourceInstrumentEnd)->SetInt(kCustomEnd);

  // "Clear unused samples" purges the lower-indexed unused sample. This is the
  // exact pool operation Project::PurgeSamples performs per unused sample.
  pool.PurgeSample(0, "TestProject");

  // The instrument correctly retargets to the shifted sample index...
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentSample)->GetInt() ==
        1);

  // ...but its custom trim points must survive: the sample is unchanged.
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentEnd)->GetInt() ==
        kCustomEnd);
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentStart)->GetInt() ==
        kCustomStart);
  CHECK(inst->FindVariable(FourCC::SampleSourceInstrumentLoopStart)->GetInt() ==
        kCustomLoopStart);
}
