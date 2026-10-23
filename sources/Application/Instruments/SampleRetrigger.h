/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SAMPLE_RETRIGGER_H_
#define _SAMPLE_RETRIGGER_H_

#include <cstdint>

class SampleRetriggerState {
public:
  void Configure(uint16_t value) {
    playbackOffset_ = static_cast<uint8_t>(value >> 8);
    interval_ = static_cast<uint8_t>(value & 0xFF);
    ticksRemaining_ = interval_;
  }

  void Reset() { Configure(0); }

  bool AdvanceTick() {
    if (interval_ == 0) {
      return false;
    }

    if (ticksRemaining_ == 0) {
      ticksRemaining_ = interval_;
      --ticksRemaining_;
      return true;
    }

    --ticksRemaining_;
    return false;
  }

  bool IsActive() const { return interval_ != 0; }

  int32_t CalculateSampleOffset(float tickSampleCount) const {
    const int16_t tickOffset =
        static_cast<int16_t>(playbackOffset_) - interval_;
    return static_cast<int32_t>(tickOffset * tickSampleCount);
  }

private:
  uint8_t playbackOffset_ = 0;
  uint8_t interval_ = 0;
  uint8_t ticksRemaining_ = 0;
};

inline void ApplySampleRetrigger(float &position, float playbackSpeed,
                                 float tickSampleCount,
                                 const SampleRetriggerState &retrigger) {
  position += retrigger.CalculateSampleOffset(tickSampleCount) * playbackSpeed;
  if (position < 0.0f) {
    position = 0.0f;
  }
}

#endif
