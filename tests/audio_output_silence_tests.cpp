/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#include "Services/Audio/AudioDriver.h"
#include "doctest/doctest.h"
#include <cstring>

namespace {
class TestOutputDriver : public AudioDriver {
public:
  explicit TestOutputDriver(AudioSettings &settings) : AudioDriver(settings) {}
  bool InitDriver() override { return true; }
  void CloseDriver() override {}
  bool StartDriver() override { return true; }
  void StopDriver() override {}
  bool Interlaced() override { return true; }
  int GetPlayedBufferPercentage() override { return 0; }
  double GetStreamTime() override { return 0; }
  const AudioBufferData &Buffer(int index) const { return pool_[index]; }
  void Consume(int index) { pool_[index].empty_ = true; }
};
} // namespace

TEST_CASE("Software output silence preserves source samples and queue timing") {
  AudioSettings settings{};
  TestOutputDriver driver(settings);
  REQUIRE(driver.Init());
  REQUIRE(driver.Start());
  short source[] = {100, -200, 300, -400};
  const short original[] = {100, -200, 300, -400};
  const short silence[] = {0, 0, 0, 0};

  driver.AddBuffer(source, 2);
  CHECK(std::memcmp(driver.Buffer(0).buffer_, original, sizeof(original)) == 0);
  driver.AddBuffer(source, 2, true);
  CHECK(std::memcmp(driver.Buffer(1).buffer_, silence, sizeof(silence)) == 0);
  CHECK(driver.Buffer(1).size_ == sizeof(source));
  CHECK_FALSE(driver.Buffer(1).empty_);
  CHECK(std::memcmp(source, original, sizeof(original)) == 0);
  // Silencing the next buffer does not overwrite a buffer already in flight.
  CHECK(std::memcmp(driver.Buffer(0).buffer_, original, sizeof(original)) == 0);

  driver.Consume(0);
  driver.AddBuffer(source, 2, false);
  CHECK(std::memcmp(driver.Buffer(0).buffer_, original, sizeof(original)) == 0);
  CHECK(driver.Buffer(0).size_ == sizeof(source));
  CHECK_FALSE(driver.Buffer(0).empty_);

  driver.Consume(1);
  driver.Stop();
  driver.AddBuffer(source, 2, true);
  CHECK(driver.Buffer(1).empty_);
}
