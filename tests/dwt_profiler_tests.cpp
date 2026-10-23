/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "System/Profiler/DWTProfiler.h"

TEST_CASE("DWT slot budget divides the buffer slot across parallel FX buses") {
  DWTProfileStats stats = {};
  stats.sampleCount_ = 1;
  stats.totalSamples_ = 441;
  stats.lastSampleRate_ = 44100;
  stats.lastVoiceCount_ = 3;

  const DWTSlotBudgetStats slotBudget =
      ComputeDWTSlotBudget(stats, 480000000U, 800000U);

  CHECK(slotBudget.slotUsX100 == 333333U);
  CHECK(slotBudget.slotPctX10 == 500U);
}

TEST_CASE(
    "DWT slot budget uses the full buffer slot for single-instance paths") {
  DWTProfileStats stats = {};
  stats.sampleCount_ = 1;
  stats.totalSamples_ = 441;
  stats.lastSampleRate_ = 44100;
  stats.lastVoiceCount_ = 1;

  const DWTSlotBudgetStats slotBudget =
      ComputeDWTSlotBudget(stats, 480000000U, 800000U);

  CHECK(slotBudget.slotUsX100 == 1000000U);
  CHECK(slotBudget.slotPctX10 == 166U);
}

TEST_CASE("DWT slot budget honors an explicit slot count override") {
  DWTProfileStats stats = {};
  stats.sampleCount_ = 1;
  stats.totalSamples_ = 441;
  stats.lastSampleRate_ = 44100;
  stats.lastVoiceCount_ = 3;
  stats.slotCountOverride_ = 1;

  const DWTSlotBudgetStats slotBudget =
      ComputeDWTSlotBudget(stats, 480000000U, 800000U);

  CHECK(slotBudget.slotUsX100 == 1000000U);
  CHECK(slotBudget.slotPctX10 == 166U);
}

TEST_CASE("DWT slot budget supports UI frame cadence profiling") {
  DWTProfileStats stats = {};
  stats.sampleCount_ = 3;
  stats.totalSamples_ = 3;
  stats.lastSampleRate_ = 30;
  stats.lastVoiceCount_ = 1;

  const DWTSlotBudgetStats slotBudget =
      ComputeDWTSlotBudget(stats, 480000000U, 4800000U);

  CHECK(slotBudget.slotUsX100 == 3333333U);
  CHECK(slotBudget.slotPctX10 == 300U);
}

TEST_CASE("DWT profiler red thresholds match the configured counters") {
  CHECK(GetProfilerRedThresholdX10(" voice source") == 250U);
  CHECK(GetProfilerRedThresholdX10(" voice chain") == 350U);
  CHECK(GetProfilerRedThresholdX10("  voice insert") == 100U);
  CHECK(GetProfilerRedThresholdX10("voice total") == 600U);
  CHECK(GetProfilerRedThresholdX10(" fx1 total") == 250U);
  CHECK(GetProfilerRedThresholdX10("out fx total") == 250U);
  CHECK(GetProfilerRedThresholdX10("grand total") == 850U);
  CHECK(GetProfilerRedThresholdX10("something else") == 0U);

  CHECK_FALSE(ShouldColorProfilerLineRed(" voice source", 250U));
  CHECK(ShouldColorProfilerLineRed(" voice source", 251U));
  CHECK_FALSE(ShouldColorProfilerLineRed(" voice chain", 350U));
  CHECK(ShouldColorProfilerLineRed(" voice chain", 351U));
  CHECK_FALSE(ShouldColorProfilerLineRed("  voice insert", 100U));
  CHECK(ShouldColorProfilerLineRed("  voice insert", 101U));
  CHECK(ShouldColorProfilerLineRed(" insert fx1", 101U));
  CHECK_FALSE(ShouldColorProfilerLineRed("out fx total", 250U));
  CHECK(ShouldColorProfilerLineRed("out fx total", 251U));
  CHECK_FALSE(ShouldColorProfilerLineRed("grand total", 850U));
  CHECK(ShouldColorProfilerLineRed("grand total", 851U));

  CHECK(ShouldColorProfilerLineGreen(" voice source", 250U));
  CHECK_FALSE(ShouldColorProfilerLineGreen(" voice source", 251U));
  CHECK(ShouldColorProfilerLineGreen(" voice chain", 350U));
  CHECK_FALSE(ShouldColorProfilerLineGreen(" voice chain", 351U));
  CHECK(ShouldColorProfilerLineGreen("grand total", 850U));
  CHECK_FALSE(ShouldColorProfilerLineGreen("grand total", 851U));
  CHECK_FALSE(ShouldColorProfilerLineGreen("something else", 0U));

  CHECK(doctest::String(GetProfilerUseColorStart(" voice source", 250U)) ==
        doctest::String("\x1b[32m"));
  CHECK(doctest::String(GetProfilerUseColorStart(" voice source", 251U)) ==
        doctest::String("\x1b[31m"));
  CHECK(doctest::String(GetProfilerUseColorStart("something else", 0U)) ==
        doctest::String(""));
  CHECK(doctest::String(GetProfilerUseColorEnd(" voice source", 250U)) ==
        doctest::String("\x1b[0m"));
  CHECK(doctest::String(GetProfilerUseColorEnd("something else", 0U)) ==
        doctest::String(""));
}
