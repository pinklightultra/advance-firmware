/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DWT_PROFILER_H_
#define _DWT_PROFILER_H_

#include <cstdint>
#include <cstdio>
#include <cstring>

#ifndef ENABLE_PROFILING
#define ENABLE_PROFILING 0
#endif

#include "Application/Model/Song.h"

#if !defined(HOST_TEST) && ENABLE_PROFILING
#include "Adapters/adv/platform/platform.h"
#endif

struct DWTProfileStats {
  const char *name_ = nullptr;
  bool printLast_ = false;
  uint16_t displayOrder_ = 0;
  uint16_t slotCountOverride_ = 0;
  uint64_t totalCycles_ = 0;
  uint64_t totalSamples_ = 0;
  uint64_t totalCpi_ = 0;
  uint64_t totalLsu_ = 0;
  uint64_t totalExc_ = 0;
  uint64_t totalFold_ = 0;
  uint32_t sampleCount_ = 0;
  uint32_t maxCycles_ = 0;
  uint32_t lastSampleRate_ = 0;
  uint32_t lastVoiceCount_ = 0;
  const DWTProfileStats *aggregateChildren_[4] = {};
  uint8_t aggregateChildCount_ = 0;
  DWTProfileStats *next_ = nullptr;
  bool registered_ = false;
};

struct DWTSlotBudgetStats {
  uint32_t slotUsX100 = 0;
  uint32_t slotPctX10 = 0;
};

inline DWTSlotBudgetStats ComputeDWTSlotBudget(const DWTProfileStats &stats,
                                               uint32_t cpuHz,
                                               uint32_t avgCycles) {
  DWTSlotBudgetStats slotBudget;
  const uint32_t slotCount = stats.slotCountOverride_ > 0
                                 ? stats.slotCountOverride_
                                 : stats.lastVoiceCount_;

  if (stats.sampleCount_ == 0 || stats.totalSamples_ == 0 ||
      stats.lastSampleRate_ == 0 || slotCount == 0) {
    return slotBudget;
  }

  const uint64_t avgSamples =
      stats.totalSamples_ / static_cast<uint64_t>(stats.sampleCount_);
  slotBudget.slotUsX100 = static_cast<uint32_t>(
      (avgSamples * 100000000ULL) /
      (static_cast<uint64_t>(stats.lastSampleRate_) * slotCount));

  const uint64_t slotCycles =
      (static_cast<uint64_t>(cpuHz) * avgSamples) /
      (static_cast<uint64_t>(stats.lastSampleRate_) * slotCount);
  if (slotCycles > 0) {
    slotBudget.slotPctX10 = static_cast<uint32_t>(
        (static_cast<uint64_t>(avgCycles) * 1000ULL) / slotCycles);
  }

  return slotBudget;
}

inline const char *TrimProfilerName(const char *name) {
  if (!name) {
    return "";
  }

  while (*name == ' ') {
    ++name;
  }
  return name;
}

inline bool ProfilerNameEquals(const char *name, const char *expected) {
  return std::strcmp(TrimProfilerName(name), expected) == 0;
}

inline bool ProfilerNameStartsWith(const char *name, const char *prefix) {
  const char *trimmed = TrimProfilerName(name);
  const size_t prefixLength = std::strlen(prefix);
  return std::strncmp(trimmed, prefix, prefixLength) == 0;
}

inline uint32_t GetProfilerRedThresholdX10(const char *name) {
  if (ProfilerNameEquals(name, "voice source")) {
    return 250;
  }
  if (ProfilerNameEquals(name, "voice chain")) {
    return 350;
  }
  if (ProfilerNameEquals(name, "voice insert") ||
      ProfilerNameStartsWith(name, "insert fx")) {
    return 100;
  }
  if (ProfilerNameEquals(name, "voice total")) {
    return 600;
  }
  if (ProfilerNameEquals(name, "fx1 total") ||
      ProfilerNameEquals(name, "fx2 total") ||
      ProfilerNameEquals(name, "fx3 total") ||
      ProfilerNameEquals(name, "out fx total")) {
    return 250;
  }
  if (ProfilerNameEquals(name, "grand total")) {
    return 850;
  }

  return 0;
}

inline bool ShouldColorProfilerLineRed(const char *name, uint32_t slotPctX10) {
  const uint32_t threshold = GetProfilerRedThresholdX10(name);
  return threshold > 0 && slotPctX10 > threshold;
}

inline bool ShouldColorProfilerLineGreen(const char *name,
                                         uint32_t slotPctX10) {
  const uint32_t threshold = GetProfilerRedThresholdX10(name);
  return threshold > 0 && slotPctX10 <= threshold;
}

inline const char *GetProfilerUseColorStart(const char *name,
                                            uint32_t slotPctX10) {
  if (ShouldColorProfilerLineRed(name, slotPctX10)) {
    return "\x1b[31m";
  }
  if (ShouldColorProfilerLineGreen(name, slotPctX10)) {
    return "\x1b[32m";
  }
  return "";
}

inline const char *GetProfilerUseColorEnd(const char *name,
                                          uint32_t slotPctX10) {
  return GetProfilerUseColorStart(name, slotPctX10)[0] != '\0' ? "\x1b[0m" : "";
}

#if !defined(HOST_TEST) && ENABLE_PROFILING
namespace DWTProfiler {
constexpr uint32_t kReportIntervalUs = 5000000;

inline DWTProfileStats *&statsHead() {
  static DWTProfileStats *head = nullptr;
  return head;
}

inline DWTProfileStats *&statsTail() {
  static DWTProfileStats *tail = nullptr;
  return tail;
}

inline uint32_t &reportStartMicros() {
  static uint32_t start = 0;
  return start;
}

inline bool &flushInProgress() {
  static bool active = false;
  return active;
}

inline bool &flushPending() {
  static bool pending = false;
  return pending;
}

inline uint32_t &activeScopeCount() {
  static uint32_t count = 0;
  return count;
}

struct Snapshot {
  uint32_t cycles_;
  uint8_t cpi_;
  uint8_t exc_;
  uint8_t lsu_;
  uint8_t fold_;
};

inline bool ensureReady() {
  static bool initialized = false;
  static bool available = false;

  if (initialized) {
    return available;
  }

  initialized = true;
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55;

  if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) {
    return false;
  }

  DWT->CYCCNT = 0;
  DWT->CPICNT = 0;
  DWT->EXCCNT = 0;
  DWT->LSUCNT = 0;
  DWT->FOLDCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk | DWT_CTRL_CPIEVTENA_Msk |
               DWT_CTRL_EXCEVTENA_Msk | DWT_CTRL_LSUEVTENA_Msk |
               DWT_CTRL_FOLDEVTENA_Msk;
  available = true;
  return true;
}

inline Snapshot snapshot() {
  return {DWT->CYCCNT, static_cast<uint8_t>(DWT->CPICNT),
          static_cast<uint8_t>(DWT->EXCCNT), static_cast<uint8_t>(DWT->LSUCNT),
          static_cast<uint8_t>(DWT->FOLDCNT)};
}

inline uint32_t diff32(uint32_t newer, uint32_t older) { return newer - older; }

inline uint32_t diff8(uint8_t newer, uint8_t older) {
  return static_cast<uint8_t>(newer - older);
}

inline void registerStats(DWTProfileStats &stats) {
  if (stats.registered_) {
    return;
  }

  stats.registered_ = true;
  stats.next_ = nullptr;

  if (!statsHead()) {
    statsHead() = &stats;
    statsTail() = &stats;
    return;
  }

  statsTail()->next_ = &stats;
  statsTail() = &stats;
}

inline void resetStats(DWTProfileStats &stats) {
  stats.totalCycles_ = 0;
  stats.totalSamples_ = 0;
  stats.totalCpi_ = 0;
  stats.totalExc_ = 0;
  stats.totalLsu_ = 0;
  stats.totalFold_ = 0;
  stats.sampleCount_ = 0;
  stats.maxCycles_ = 0;
  stats.lastSampleRate_ = 0;
  stats.lastVoiceCount_ = 0;
}

struct DWTDisplayStats {
  uint32_t avgCycles = 0;
  uint32_t avgCyclesPerSample = 0;
  uint32_t avgCpi = 0;
  uint32_t avgExc = 0;
  uint32_t avgLsu = 0;
  uint32_t avgFold = 0;
  uint32_t avgUsX100 = 0;
  uint32_t maxUsX100 = 0;
  uint32_t slotUsX100 = 0;
  uint32_t slotPctX10 = 0;
};

struct DWTResolvedStats {
  uint64_t totalCycles = 0;
  uint64_t totalSamples = 0;
  uint64_t totalCpi = 0;
  uint64_t totalLsu = 0;
  uint64_t totalExc = 0;
  uint64_t totalFold = 0;
  uint32_t sampleCount = 0;
  uint32_t maxCycles = 0;
  uint32_t lastSampleRate = 0;
  uint32_t slotCount = 0;
};

inline DWTSlotBudgetStats ComputeDWTSlotBudget(const DWTResolvedStats &stats,
                                               uint32_t cpuHz,
                                               uint32_t avgCycles) {
  DWTSlotBudgetStats slotBudget;

  if (stats.sampleCount == 0 || stats.totalSamples == 0 ||
      stats.lastSampleRate == 0 || stats.slotCount == 0) {
    return slotBudget;
  }

  const uint64_t avgSamples =
      stats.totalSamples / static_cast<uint64_t>(stats.sampleCount);
  slotBudget.slotUsX100 = static_cast<uint32_t>(
      (avgSamples * 100000000ULL) /
      (static_cast<uint64_t>(stats.lastSampleRate) * stats.slotCount));

  const uint64_t slotCycles =
      (static_cast<uint64_t>(cpuHz) * avgSamples) /
      (static_cast<uint64_t>(stats.lastSampleRate) * stats.slotCount);
  if (slotCycles > 0) {
    slotBudget.slotPctX10 = static_cast<uint32_t>(
        (static_cast<uint64_t>(avgCycles) * 1000ULL) / slotCycles);
  }

  return slotBudget;
}

inline DWTResolvedStats resolveStatsForDisplay(const DWTProfileStats &stats) {
  DWTResolvedStats resolved;

  if (stats.aggregateChildCount_ == 0) {
    resolved.totalCycles = stats.totalCycles_;
    resolved.totalSamples = stats.totalSamples_;
    resolved.totalCpi = stats.totalCpi_;
    resolved.totalLsu = stats.totalLsu_;
    resolved.totalExc = stats.totalExc_;
    resolved.totalFold = stats.totalFold_;
    resolved.sampleCount = stats.sampleCount_;
    resolved.maxCycles = stats.maxCycles_;
    resolved.lastSampleRate = stats.lastSampleRate_;
    resolved.slotCount = stats.slotCountOverride_ > 0 ? stats.slotCountOverride_
                                                      : stats.lastVoiceCount_;
    return resolved;
  }

  for (uint8_t i = 0; i < stats.aggregateChildCount_; ++i) {
    const DWTProfileStats *child = stats.aggregateChildren_[i];
    if (!child) {
      continue;
    }

    const DWTResolvedStats childResolved = resolveStatsForDisplay(*child);
    resolved.totalCycles += childResolved.totalCycles;
    resolved.totalCpi += childResolved.totalCpi;
    resolved.totalLsu += childResolved.totalLsu;
    resolved.totalExc += childResolved.totalExc;
    resolved.totalFold += childResolved.totalFold;
    resolved.maxCycles += childResolved.maxCycles;
    if (childResolved.totalSamples > resolved.totalSamples) {
      resolved.totalSamples = childResolved.totalSamples;
    }
    if (childResolved.sampleCount > resolved.sampleCount) {
      resolved.sampleCount = childResolved.sampleCount;
    }
    if (resolved.lastSampleRate == 0 && childResolved.lastSampleRate > 0) {
      resolved.lastSampleRate = childResolved.lastSampleRate;
    }
    if (resolved.slotCount == 0 && childResolved.slotCount > 0) {
      resolved.slotCount = childResolved.slotCount;
    }
  }

  if (stats.slotCountOverride_ > 0) {
    resolved.slotCount = stats.slotCountOverride_;
  }

  return resolved;
}

inline DWTDisplayStats computeDisplayStats(const DWTResolvedStats &stats) {
  DWTDisplayStats display;
  const uint32_t cpuHz = SystemCoreClock > 0 ? SystemCoreClock : 1U;

  if (stats.sampleCount > 0) {
    display.avgCycles =
        static_cast<uint32_t>(stats.totalCycles / stats.sampleCount);
    display.avgCyclesPerSample =
        stats.totalSamples > 0
            ? static_cast<uint32_t>(stats.totalCycles / stats.totalSamples)
            : 0;
    display.avgCpi = static_cast<uint32_t>(stats.totalCpi / stats.sampleCount);
    display.avgExc = static_cast<uint32_t>(stats.totalExc / stats.sampleCount);
    display.avgLsu = static_cast<uint32_t>(stats.totalLsu / stats.sampleCount);
    display.avgFold =
        static_cast<uint32_t>(stats.totalFold / stats.sampleCount);
    display.avgUsX100 = static_cast<uint32_t>(
        (stats.totalCycles * 100000000ULL) /
        (static_cast<uint64_t>(stats.sampleCount) * cpuHz));
    display.maxUsX100 = static_cast<uint32_t>(
        (static_cast<uint64_t>(stats.maxCycles) * 100000000ULL) / cpuHz);
  }

  const DWTSlotBudgetStats slotBudget =
      ComputeDWTSlotBudget(stats, cpuHz, display.avgCycles);
  display.slotUsX100 = slotBudget.slotUsX100;
  display.slotPctX10 = slotBudget.slotPctX10;

  return display;
}

/*
 * Profiler fields after cyc= in the log output:
 *  cps  average cycles per audio sample (totalCycles_ / totalSamples_).
 *  cpi  average DWT CPICNT delta per profiled scope. ARM names this the CPI
 *       counter, but it is not a literal cycles-per-instruction ratio. It
 *       counts extra cycles for multi-cycle non-load/store instructions plus
 *       instruction fetch stalls.
 *  lsu  average DWT LSUCNT delta per profiled scope. This counts extra cycles
 *       spent servicing load/store instructions beyond their first cycle.
 *  exc  average DWT EXCCNT delta per profiled scope. This is exception
 *       overhead such as entry/exit stacking and pre-emption.
 *  fold average DWT FOLDCNT delta per profiled scope. This counts folded
 *       instructions, meaning instructions executed in 0 cycles.
 *  n    number of profiled scopes aggregated into the printed report window.
 */
inline void printDisplayLine(const char *name, const DWTDisplayStats &display,
                             uint32_t sampleCount) {
  const char *useColorStart =
      GetProfilerUseColorStart(name, display.slotPctX10);
  const char *useColorEnd = GetProfilerUseColorEnd(name, display.slotPctX10);
  printf("[DWT] %-14s avg=%5lu.%02lu us max=%5lu.%02lu us slot=%5lu.%02lu us "
         "use=%s%3lu.%01lu%%%s cyc=%6lu cps=%4lu cpi=%3lu lsu=%3lu exc=%3lu "
         "fold=%3lu n=%lu\r\n",
         name ? name : "unnamed", (unsigned long)(display.avgUsX100 / 100),
         (unsigned long)(display.avgUsX100 % 100),
         (unsigned long)(display.maxUsX100 / 100),
         (unsigned long)(display.maxUsX100 % 100),
         (unsigned long)(display.slotUsX100 / 100),
         (unsigned long)(display.slotUsX100 % 100), useColorStart,
         (unsigned long)(display.slotPctX10 / 10),
         (unsigned long)(display.slotPctX10 % 10), useColorEnd,
         (unsigned long)display.avgCycles,
         (unsigned long)display.avgCyclesPerSample,
         (unsigned long)display.avgCpi, (unsigned long)display.avgLsu,
         (unsigned long)display.avgExc, (unsigned long)display.avgFold,
         (unsigned long)sampleCount);
}

inline void printStatsLine(const DWTProfileStats &stats) {
  const DWTResolvedStats resolved = resolveStatsForDisplay(stats);
  printDisplayLine(stats.name_, computeDisplayStats(resolved),
                   resolved.sampleCount);
}

inline void accumulateSample(DWTProfileStats &stats, uint32_t cycles,
                             uint32_t cpi, uint32_t exc, uint32_t lsu,
                             uint32_t fold, bool countSample, int samplecount,
                             int sampleRate, int voiceCount) {
  stats.totalCycles_ += cycles;
  if (countSample) {
    stats.totalSamples_ +=
        samplecount > 0 ? static_cast<uint32_t>(samplecount) : 0;
    stats.sampleCount_++;
  }
  stats.totalCpi_ += cpi;
  stats.totalExc_ += exc;
  stats.totalLsu_ += lsu;
  stats.totalFold_ += fold;
  if (sampleRate > 0) {
    stats.lastSampleRate_ = static_cast<uint32_t>(sampleRate);
  }
  if (voiceCount > 0) {
    stats.lastVoiceCount_ = static_cast<uint32_t>(voiceCount);
  }
  if (countSample && cycles > stats.maxCycles_) {
    stats.maxCycles_ = cycles;
  }
}

inline void collectOrderedStats(DWTProfileStats **statsArray, size_t &count,
                                size_t capacity) {
  count = 0;
  for (DWTProfileStats *stats = statsHead(); stats; stats = stats->next_) {
    if (count >= capacity) {
      break;
    }
    statsArray[count++] = stats;
  }
}

inline bool shouldSortAfter(const DWTProfileStats &lhs,
                            const DWTProfileStats &rhs) {
  if (lhs.printLast_ != rhs.printLast_) {
    return lhs.printLast_ && !rhs.printLast_;
  }
  return lhs.displayOrder_ > rhs.displayOrder_;
}

inline void flushAll(uint32_t now) {
  if (flushInProgress()) {
    return;
  }

  constexpr size_t kMaxRegisteredStats = 64;
  DWTProfileStats *orderedStats[kMaxRegisteredStats];
  size_t orderedCount = 0;
  collectOrderedStats(orderedStats, orderedCount, kMaxRegisteredStats);
  for (size_t i = 1; i < orderedCount; ++i) {
    DWTProfileStats *key = orderedStats[i];
    size_t j = i;
    while (j > 0 && shouldSortAfter(*orderedStats[j - 1], *key)) {
      orderedStats[j] = orderedStats[j - 1];
      --j;
    }
    orderedStats[j] = key;
  }

  flushInProgress() = true;
  printf("[DWT] ----------------------------------------\r\n");
  for (size_t i = 0; i < orderedCount; ++i) {
    printStatsLine(*orderedStats[i]);
  }

  for (DWTProfileStats *stats = statsHead(); stats; stats = stats->next_) {
    resetStats(*stats);
  }

  reportStartMicros() = now;
  flushPending() = false;
  flushInProgress() = false;
}

inline void enterScope() { activeScopeCount()++; }

inline void leaveScope() {
  if (activeScopeCount() == 0) {
    return;
  }

  activeScopeCount()--;
  if (activeScopeCount() == 0 && flushPending()) {
    flushAll(micros());
  }
}

inline void record(DWTProfileStats &stats, const Snapshot &start,
                   const Snapshot &end, int samplecount, int sampleRate,
                   int voiceCount) {
  registerStats(stats);

  const uint32_t cycles = diff32(end.cycles_, start.cycles_);
  const uint32_t cpi = diff8(end.cpi_, start.cpi_);
  const uint32_t exc = diff8(end.exc_, start.exc_);
  const uint32_t lsu = diff8(end.lsu_, start.lsu_);
  const uint32_t fold = diff8(end.fold_, start.fold_);
  const uint32_t now = micros();

  if (reportStartMicros() == 0) {
    reportStartMicros() = now;
  }

  accumulateSample(stats, cycles, cpi, exc, lsu, fold, true, samplecount,
                   sampleRate, voiceCount);

  if ((now - reportStartMicros()) < kReportIntervalUs) {
    return;
  }

  flushPending() = true;
}

inline void recordTotals(DWTProfileStats &stats, uint32_t cycles, uint32_t cpi,
                         uint32_t exc, uint32_t lsu, uint32_t fold,
                         int samplecount, int sampleRate, int voiceCount,
                         bool countSample = true) {
  registerStats(stats);

  const uint32_t now = micros();

  if (reportStartMicros() == 0) {
    reportStartMicros() = now;
  }

  accumulateSample(stats, cycles, cpi, exc, lsu, fold, countSample, samplecount,
                   sampleRate, voiceCount);

  if ((now - reportStartMicros()) < kReportIntervalUs) {
    return;
  }

  flushPending() = true;
}
} // namespace DWTProfiler

class ScopedDWTProfile {
public:
  ScopedDWTProfile(DWTProfileStats *stats, int samplecount, int sampleRate,
                   int voiceCount, DWTProfileStats *secondaryStats = nullptr,
                   bool countSecondarySample = true)
      : stats_(stats), samplecount_(samplecount), sampleRate_(sampleRate),
        voiceCount_(voiceCount), active_(false),
        secondaryStats_(secondaryStats),
        countSecondarySample_(countSecondarySample) {
    if (!stats_ || !DWTProfiler::ensureReady()) {
      return;
    }

    DWTProfiler::enterScope();
    start_ = DWTProfiler::snapshot();
    active_ = true;
  }

  ~ScopedDWTProfile() {
    if (!active_) {
      return;
    }

    const DWTProfiler::Snapshot end = DWTProfiler::snapshot();
    DWTProfiler::record(*stats_, start_, end, samplecount_, sampleRate_,
                        voiceCount_);
    if (secondaryStats_) {
      DWTProfiler::recordTotals(
          *secondaryStats_, DWTProfiler::diff32(end.cycles_, start_.cycles_),
          DWTProfiler::diff8(end.cpi_, start_.cpi_),
          DWTProfiler::diff8(end.exc_, start_.exc_),
          DWTProfiler::diff8(end.lsu_, start_.lsu_),
          DWTProfiler::diff8(end.fold_, start_.fold_), samplecount_,
          sampleRate_, voiceCount_, countSecondarySample_);
    }
    DWTProfiler::leaveScope();
  }

private:
  DWTProfileStats *stats_;
  int samplecount_;
  int sampleRate_;
  int voiceCount_;
  bool active_;
  DWTProfileStats *secondaryStats_;
  bool countSecondarySample_;
  DWTProfiler::Snapshot start_{};
};

class AccumulatingDWTProfile {
public:
  AccumulatingDWTProfile(DWTProfileStats *stats, int samplecount,
                         int sampleRate, int voiceCount)
      : stats_(stats), samplecount_(samplecount), sampleRate_(sampleRate),
        voiceCount_(voiceCount), active_(false), segmentActive_(false),
        hasSamples_(false), cycles_(0), cpi_(0), exc_(0), lsu_(0), fold_(0) {
    if (!stats_ || !DWTProfiler::ensureReady()) {
      return;
    }

    DWTProfiler::enterScope();
    active_ = true;
  }

  void Begin() {
    if (!active_ || segmentActive_) {
      return;
    }

    segmentStart_ = DWTProfiler::snapshot();
    segmentActive_ = true;
  }

  void End() {
    if (!active_ || !segmentActive_) {
      return;
    }

    const DWTProfiler::Snapshot end = DWTProfiler::snapshot();
    cycles_ += DWTProfiler::diff32(end.cycles_, segmentStart_.cycles_);
    cpi_ += DWTProfiler::diff8(end.cpi_, segmentStart_.cpi_);
    exc_ += DWTProfiler::diff8(end.exc_, segmentStart_.exc_);
    lsu_ += DWTProfiler::diff8(end.lsu_, segmentStart_.lsu_);
    fold_ += DWTProfiler::diff8(end.fold_, segmentStart_.fold_);
    segmentActive_ = false;
    hasSamples_ = true;
  }

  ~AccumulatingDWTProfile() {
    if (!active_) {
      return;
    }

    if (segmentActive_) {
      End();
    }
    if (hasSamples_) {
      DWTProfiler::recordTotals(*stats_, cycles_, cpi_, exc_, lsu_, fold_,
                                samplecount_, sampleRate_, voiceCount_);
    }
    DWTProfiler::leaveScope();
  }

private:
  DWTProfileStats *stats_;
  int samplecount_;
  int sampleRate_;
  int voiceCount_;
  bool active_;
  bool segmentActive_;
  bool hasSamples_;
  uint32_t cycles_;
  uint32_t cpi_;
  uint32_t exc_;
  uint32_t lsu_;
  uint32_t fold_;
  DWTProfiler::Snapshot segmentStart_{};
};

inline void RegisterDWTProfileStats(DWTProfileStats *stats) {
  if (!stats || !DWTProfiler::ensureReady()) {
    return;
  }

  DWTProfiler::registerStats(*stats);
}
#else
class ScopedDWTProfile {
public:
  ScopedDWTProfile(DWTProfileStats *stats, int samplecount, int sampleRate,
                   int voiceCount, DWTProfileStats *secondaryStats = nullptr,
                   bool countSecondarySample = true) {
    (void)stats;
    (void)samplecount;
    (void)sampleRate;
    (void)voiceCount;
    (void)secondaryStats;
    (void)countSecondarySample;
  }
};

class AccumulatingDWTProfile {
public:
  AccumulatingDWTProfile(DWTProfileStats *stats, int samplecount,
                         int sampleRate, int voiceCount) {
    (void)stats;
    (void)samplecount;
    (void)sampleRate;
    (void)voiceCount;
  }

  void Begin() {}
  void End() {}
};

inline void RegisterDWTProfileStats(DWTProfileStats *stats) { (void)stats; }
#endif

#endif
