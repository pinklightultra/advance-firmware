/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Adapters/adv/system/SamplePoolStorage.h"
#include "Application/Instruments/SamplePool.h"
#include "doctest/doctest.h"

TEST_CASE("Import sample size uses the converted rate and bit depth") {
  uint32_t sampleSize = 0;

  REQUIRE(SamplePool::CalculateImportSampleSize(384000, 384000, 1, sampleSize));
  CHECK(sampleSize == 44100 * sizeof(int16_t));

  REQUIRE(SamplePool::CalculateImportSampleSize(384000, 384000, 2, sampleSize));
  CHECK(sampleSize == 44100 * 2 * sizeof(int16_t));
}

TEST_CASE("Import sample size rounds partial output frames up") {
  uint32_t sampleSize = 0;

  REQUIRE(SamplePool::CalculateImportSampleSize(48001, 48000, 1, sampleSize));
  CHECK(sampleSize == 44101 * sizeof(int16_t));
}

TEST_CASE("Import sample size rejects invalid and overflowing sizes") {
  uint32_t sampleSize = 0;

  CHECK_FALSE(SamplePool::CalculateImportSampleSize(0, 44100, 1, sampleSize));
  CHECK_FALSE(SamplePool::CalculateImportSampleSize(1, 0, 1, sampleSize));
  CHECK_FALSE(SamplePool::CalculateImportSampleSize(1, 44100, 0, sampleSize));
  CHECK_FALSE(
      SamplePool::CalculateImportSampleSize(UINT32_MAX, 44100, 2, sampleSize));
}

TEST_CASE("Sample storage capacity includes alignment padding") {
  CHECK(AdvanceSampleStorage::GetAvailableCapacity(0, 16) == 16);
  CHECK(AdvanceSampleStorage::GetAvailableCapacity(1, 16) == 12);
  CHECK(AdvanceSampleStorage::Fits(1, 16, 12));
  CHECK_FALSE(AdvanceSampleStorage::Fits(1, 16, 13));
  CHECK(AdvanceSampleStorage::Fits(4, 16, 12));
}

TEST_CASE("A sample must fit entirely in one storage region") {
  const uint32_t available1 = AdvanceSampleStorage::GetAvailableCapacity(8, 16);
  const uint32_t available2 = AdvanceSampleStorage::GetAvailableCapacity(8, 16);
  REQUIRE(available1 + available2 == 16);

  CHECK_FALSE(AdvanceSampleStorage::FitsInEither(8, 16, 8, 16, 12));
  CHECK(AdvanceSampleStorage::FitsInEither(8, 16, 8, 16, 8));
}
