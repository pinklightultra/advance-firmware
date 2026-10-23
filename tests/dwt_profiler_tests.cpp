/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "System/Profiler/DWTProfiler.h"

TEST_CASE("DWT budget shares preserve the profiler allocation hierarchy") {
  CHECK(ComputeDWTBudgetSharePctX1000(50000U, 8U) == 6250U);
  CHECK(ComputeDWTBudgetSharePctX1000(5000U, 8U) == 625U);
  CHECK(ComputeDWTBudgetSharePctX1000(40000U, 3U) == 13333U);
  CHECK(ComputeDWTBudgetSharePctX1000(40000U, 0U) == 0U);
}

TEST_CASE("DWT percentages use the full profiling interval") {
  CHECK(ComputeDWTAbsolutePercentX1000(240000000ULL, 480000000U, 5000000U) ==
        10000U);
  CHECK(ComputeDWTAbsolutePercentX1000(2400000000ULL, 480000000U, 5000000U) ==
        100000U);
  CHECK(ComputeDWTAbsolutePercentX1000(15000000ULL, 480000000U, 5000000U) ==
        625U);
  CHECK(ComputeDWTAbsolutePercentX1000(1ULL, 0U, 5000000U) == 0U);
  CHECK(ComputeDWTAbsolutePercentX1000(1ULL, 480000000U, 0U) == 0U);
}

TEST_CASE("DWT residuals expose only unmeasured cycles") {
  CHECK(ComputeDWTResidualCycles(1000ULL, 750ULL) == 250ULL);
  CHECK(ComputeDWTResidualCycles(1000ULL, 1000ULL) == 0ULL);
  CHECK(ComputeDWTResidualCycles(1000ULL, 1250ULL) == 0ULL);
}

TEST_CASE("DWT profiler colors compare absolute use with a slot budget") {
  CHECK_FALSE(ShouldColorProfilerValueRed(625U, 625U));
  CHECK(ShouldColorProfilerValueRed(626U, 625U));
  CHECK(ShouldColorProfilerValueGreen(625U, 625U));
  CHECK_FALSE(ShouldColorProfilerValueGreen(626U, 625U));
  CHECK_FALSE(ShouldColorProfilerValueGreen(0U, 0U));

  CHECK(doctest::String(GetProfilerUseColorStart(625U, 625U)) ==
        doctest::String("\x1b[32m"));
  CHECK(doctest::String(GetProfilerUseColorStart(626U, 625U)) ==
        doctest::String("\x1b[31m"));
  CHECK(doctest::String(GetProfilerUseColorStart(10000U, 0U)) ==
        doctest::String(""));
  CHECK(doctest::String(GetProfilerUseColorEnd(625U, 625U)) ==
        doctest::String("\x1b[0m"));
  CHECK(doctest::String(GetProfilerUseColorEnd(10000U, 0U)) ==
        doctest::String(""));
}
