/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _STARLOOM_INSTRUMENT_DSP_H_
#define _STARLOOM_INSTRUMENT_DSP_H_

#include "PitchOffset.h"
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace StarLoomInstrumentDSP {

constexpr int32_t kLevelMax = 0x7F;
constexpr int32_t kPitchUnitsPerSemitone = 128;
constexpr int32_t kBraidsPitchMax = 16383;
constexpr int32_t kPcmFullScale = 32768;

enum class InteractionMode : uint8_t {
  Mix,
  Ring,
  Difference,
  Count,
};

constexpr int32_t kInteractionModeCount =
    static_cast<int32_t>(InteractionMode::Count);

constexpr int32_t ClampBraidsPitch(int32_t pitch) {
  return pitch < 0 ? 0 : (pitch > kBraidsPitchMax ? kBraidsPitchMax : pitch);
}

constexpr int32_t OffsetCentsToPitchUnits(int32_t cents) {
  const int32_t rounding = cents < 0 ? -(PitchOffset::CentsPerSemitone / 2)
                                     : PitchOffset::CentsPerSemitone / 2;
  return (cents * kPitchUnitsPerSemitone + rounding) /
         PitchOffset::CentsPerSemitone;
}

constexpr int32_t CalculatePitch(uint8_t midiNote, int32_t pitchOffsetCents,
                                 int32_t voicePitchOffset = 0) {
  const int32_t basePitch =
      kPitchUnitsPerSemitone * (static_cast<int32_t>(midiNote) - 12);
  return ClampBraidsPitch(
      basePitch + OffsetCentsToPitchUnits(pitchOffsetCents) + voicePitchOffset);
}

inline int32_t PitchMultiplierToPitchUnits(float multiplier) {
  return static_cast<int32_t>(
      std::lround(std::log2(multiplier > 0.0f ? multiplier : 1.0e-6f) * 12.0f *
                  kPitchUnitsPerSemitone));
}

constexpr int32_t ClampMix(int32_t mix) {
  return mix < -kLevelMax ? -kLevelMax : (mix > kLevelMax ? kLevelMax : mix);
}

// At the center both oscillators run at full level. Moving away from center
// attenuates only the oscillator on the opposite side of the balance.
constexpr int32_t Oscillator1LevelFromMix(int32_t mix) {
  const int32_t clampedMix = ClampMix(mix);
  return clampedMix > 0 ? kLevelMax - clampedMix : kLevelMax;
}

constexpr int32_t Oscillator2LevelFromMix(int32_t mix) {
  const int32_t clampedMix = ClampMix(mix);
  return clampedMix < 0 ? kLevelMax + clampedMix : kLevelMax;
}

constexpr size_t BraidsRenderSize(size_t outputSize) {
  return outputSize + (outputSize & 1U);
}

// Braids' zero-balance Mix attenuates positive samples by one least-significant
// bit. Preserve that behavior when the signature waveshaper is bypassed.
constexpr int16_t ApplyDisabledSignature(int16_t sample) {
  return static_cast<int16_t>(sample - (sample > 0 ? 1 : 0));
}

constexpr int32_t MixOscillators(int16_t oscillator1, int16_t oscillator2,
                                 int32_t oscillator1Level,
                                 int32_t oscillator2Level) {
  return (static_cast<int32_t>(oscillator1) * oscillator1Level +
          static_cast<int32_t>(oscillator2) * oscillator2Level) /
         kLevelMax;
}

constexpr int32_t ScaleOscillator(int16_t oscillator, int32_t level) {
  return static_cast<int32_t>(oscillator) * level / kLevelMax;
}

constexpr int32_t ApplyInteraction(int16_t oscillator1, int16_t oscillator2,
                                   int32_t oscillator1Level,
                                   int32_t oscillator2Level,
                                   InteractionMode mode) {
  const int32_t scaledOscillator1 =
      ScaleOscillator(oscillator1, oscillator1Level);
  const int32_t scaledOscillator2 =
      ScaleOscillator(oscillator2, oscillator2Level);

  switch (mode) {
  case InteractionMode::Ring:
    return scaledOscillator1 * scaledOscillator2 / kPcmFullScale;
  case InteractionMode::Difference:
    return scaledOscillator1 - scaledOscillator2;
  case InteractionMode::Mix:
  default:
    return MixOscillators(oscillator1, oscillator2, oscillator1Level,
                          oscillator2Level);
  }
}

} // namespace StarLoomInstrumentDSP

#endif
