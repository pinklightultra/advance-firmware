/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Instruments/InstrumentBank.h"
#include "Application/Instruments/SamplePool.h"
#include "Externals/etl/include/etl/variant_pool.h"
#include <cstring>

namespace {

struct DummySampleInstrument {
  explicit DummySampleInstrument(int id = 0) : id_(id) {}
  int id_;
};

struct DummyMidiInstrument {
  explicit DummyMidiInstrument(int channel = 0) : channel_(channel) {}
  int channel_;
};

using TestInstrumentPool =
    etl::variant_pool<4, DummySampleInstrument, DummyMidiInstrument>;

class LegacySelectionTestSamplePool : public SamplePool {
public:
  void Reset() override { count_ = 0; }
  bool CheckSampleFits(uint32_t) override { return true; }
  uint32_t GetAvailableSampleStorageSpace() override { return 0; }
  bool unloadSample(uint32_t index) override {
    if (index >= count_) {
      return false;
    }
    for (uint32_t i = index; i < count_ - 1; ++i) {
      std::strncpy(nameStore_[i], nameStore_[i + 1],
                   MAX_INSTRUMENT_FILENAME_LENGTH);
      nameStore_[i][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
    }
    --count_;
    SetChanged();
    SamplePoolEvent event;
    event.index_ = index;
    event.type_ = SPET_DELETE;
    NotifyObservers(&event);
    return true;
  }

  void SeedSamples() {
    static constexpr const char *sampleNames[] = {"A.wav", "B.wav", "C.wav"};
    count_ = 3;
    for (uint32_t i = 0; i < count_; ++i) {
      std::strncpy(nameStore_[i], sampleNames[i],
                   MAX_INSTRUMENT_FILENAME_LENGTH);
      nameStore_[i][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
    }
  }

protected:
  bool loadSample(const char *name) override {
    if (count_ >= MAX_SAMPLES) {
      return false;
    }
    std::strncpy(nameStore_[count_], name, MAX_INSTRUMENT_FILENAME_LENGTH);
    nameStore_[count_][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
    ++count_;
    return true;
  }
};

class SamplePoolInstallGuard {
public:
  SamplePoolInstallGuard() : previous_(SamplePool::GetInstance()) {
    SamplePool::Install(&pool_);
  }

  ~SamplePoolInstallGuard() { SamplePool::Install(previous_); }

  LegacySelectionTestSamplePool &pool() { return pool_; }

private:
  SamplePool *previous_;
  LegacySelectionTestSamplePool pool_;
};

} // namespace

TEST_CASE("Variant pool shares capacity across instrument types") {
  TestInstrumentPool pool;

  CHECK(pool.max_size() == 4);
  CHECK(pool.available() == 4);

  CHECK(pool.create<DummySampleInstrument>(1) != nullptr);
  CHECK(pool.create<DummyMidiInstrument>(2) != nullptr);
  CHECK(pool.create<DummySampleInstrument>(3) != nullptr);
  CHECK(pool.create<DummyMidiInstrument>(4) != nullptr);

  CHECK(pool.available() == 0);
  CHECK(pool.full());
  CHECK(pool.create<DummySampleInstrument>(5) == nullptr);
}

TEST_CASE("Variant pool reuses freed slots across instrument types") {
  TestInstrumentPool pool;

  DummySampleInstrument *sample = pool.create<DummySampleInstrument>(11);
  REQUIRE(sample != nullptr);
  CHECK(sample->id_ == 11);

  pool.destroy(sample);
  CHECK(pool.available() == 4);

  DummyMidiInstrument *midi = pool.create<DummyMidiInstrument>(7);
  REQUIRE(midi != nullptr);
  CHECK(midi->channel_ == 7);
  CHECK(pool.available() == 3);
}

TEST_CASE("Legacy sample selection stays enabled for its project session") {
  SamplePoolInstallGuard samplePoolGuard;
  InstrumentBank bank;

  CHECK_FALSE(bank.IsLegacySampleInstrumentSelectionEnabled());
  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE_SOURCE, 0) == 0);
  CHECK_FALSE(bank.IsLegacySampleInstrumentSelectionEnabled());

  bank.releaseInstrument(0);
  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE, 0) == 0);
  CHECK(bank.IsLegacySampleInstrumentSelectionEnabled());

  bank.releaseInstrument(0);
  CHECK(bank.IsLegacySampleInstrumentSelectionEnabled());

  bank.Reset();
  CHECK_FALSE(bank.IsLegacySampleInstrumentSelectionEnabled());
}

TEST_CASE("Sample assignments survive a project-pool sample reload") {
  SamplePoolInstallGuard samplePoolGuard;
  samplePoolGuard.pool().SeedSamples();
  InstrumentBank bank;

  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE, 0) == 0);
  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE_SOURCE, 1) == 1);
  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE_SOURCE, 2) == 2);

  auto *sample = static_cast<SampleInstrument *>(bank.GetInstrument(0));
  auto *sampleSource =
      static_cast<SampleSourceInstrument *>(bank.GetInstrument(1));
  auto *unrelated =
      static_cast<SampleSourceInstrument *>(bank.GetInstrument(2));
  sample->AssignSample(0);
  sampleSource->AssignSample(0);
  unrelated->AssignSample(1);

  const auto assignments = bank.CaptureSampleAssignments(0);
  const int32_t newIndex =
      samplePoolGuard.pool().ReloadSample(0, "/projects/demo/samples/A.wav");

  REQUIRE(newIndex == 2);
  REQUIRE(sample->GetSampleIndex() == -1);
  REQUIRE(sampleSource->GetSampleIndex() == -1);
  CHECK(sample->FindVariable(FourCC::SampleInstrumentSample)->GetListSize() ==
        3);
  CHECK(sampleSource->FindVariable(FourCC::SampleSourceInstrumentSample)
            ->GetListSize() == 3);
  bank.RestoreSampleAssignments(assignments, newIndex);

  CHECK(sample->GetSampleIndex() == 2);
  CHECK(sampleSource->GetSampleIndex() == 2);
  CHECK(sample->FindVariable(FourCC::SampleInstrumentSample)->GetString() ==
        "A.wav");
  CHECK(sampleSource->FindVariable(FourCC::SampleSourceInstrumentSample)
            ->GetString() == "A.wav");
  CHECK(unrelated->GetSampleIndex() == 0);
}

TEST_CASE(
    "Cloning a sample instrument preserves slices and custom name [#491]") {
  SamplePoolInstallGuard samplePoolGuard;
  InstrumentBank bank;

  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE, 0) == 0);
  auto *sample = static_cast<SampleInstrument *>(bank.GetInstrument(0));
  REQUIRE(sample != nullptr);

  sample->SetName("LegacySlices");
  sample->SetSlicePoint(0, 100);
  sample->SetSlicePoint(1, 500);
  sample->SetSlicePoint(2, 1200);

  const uint16_t clonedId = bank.Clone(0);
  REQUIRE(clonedId == 1);

  auto *cloned = static_cast<SampleInstrument *>(bank.GetInstrument(clonedId));
  REQUIRE(cloned != nullptr);
  CHECK(cloned->GetUserSetName() == "LegacySlices");
  CHECK(cloned->GetSlicePoint(0) == 100);
  CHECK(cloned->GetSlicePoint(1) == 500);
  CHECK(cloned->GetSlicePoint(2) == 1200);
  CHECK(cloned->GetSlicePoint(3) == 0);
}

TEST_CASE("Cloning a sample source instrument preserves slices and custom name "
          "[#491]") {
  SamplePoolInstallGuard samplePoolGuard;
  InstrumentBank bank;

  REQUIRE(bank.GetNextAndAssignID(IT_SAMPLE_SOURCE, 0) == 0);
  auto *sampleSource =
      static_cast<SampleSourceInstrument *>(bank.GetInstrument(0));
  REQUIRE(sampleSource != nullptr);

  sampleSource->SetName("SourceSlices");
  sampleSource->SetSlicePoint(0, 200);
  sampleSource->SetSlicePoint(1, 800);
  sampleSource->SetSlicePoint(2, 1500);

  const uint16_t clonedId = bank.Clone(0);
  REQUIRE(clonedId == 1);

  auto *cloned =
      static_cast<SampleSourceInstrument *>(bank.GetInstrument(clonedId));
  REQUIRE(cloned != nullptr);
  CHECK(cloned->GetUserSetName() == "SourceSlices");
  CHECK(cloned->GetSlicePoint(0) == 200);
  CHECK(cloned->GetSlicePoint(1) == 800);
  CHECK(cloned->GetSlicePoint(2) == 1500);
  CHECK(cloned->GetSlicePoint(3) == 0);
}
