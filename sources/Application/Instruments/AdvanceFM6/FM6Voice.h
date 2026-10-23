/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _FM6_VOICE_H_
#define _FM6_VOICE_H_

#include "FM6Dx7Patch.h"

#include "Externals/etl/include/etl/array.h"
#include <cstddef>
#include <cstdint>

namespace fm6 {

static constexpr size_t RenderBlockSampleCount = 64;
static constexpr uint8_t DefaultVelocity = 100;

struct Dx7EnvelopeState {
  int32_t levelQ24 = 0;
  int32_t targetLevelQ24 = 0;
  uint32_t incrementQ24 = 0;
  int32_t outputLevel = 0;
  uint8_t rateScaling = 0;
  uint8_t stageIndex = 0;
  bool rising = false;
  bool keyDown = false;
};

struct OperatorState {
  uint32_t phase = 0;
  uint32_t phaseStep = 0;
  // Note-constant base pitch (Q24 log-frequency). The per-block phaseStep is
  // derived from this plus the shared pitch modulation each render block.
  int32_t baseLogFreq = 0;
  int32_t gainQ24 = 0;
  Dx7EnvelopeState envelope{};
};

// Runtime state for the global pitch envelope generator. Output is a signed
// log-frequency offset (Q24, 1<<24 per octave) applied to every operator.
struct PitchEnvState {
  int32_t level = 0;
  int32_t targetLevel = 0;
  int32_t increment = 0;
  uint8_t stageIndex = 0;
  bool rising = false;
  bool keyDown = false;
};

// Runtime state for the global LFO. Ticked once per render block.
struct LfoState {
  uint32_t phase = 0;
  uint32_t delta = 0;
  uint32_t delayIncrement = 0;
  uint32_t delayIncrement2 = 0;
  uint32_t delayState = 0;
  uint8_t waveform = 0;
  uint8_t randState = 0;
  bool sync = false;
};

struct VoiceState {
  bool active = false;
  uint8_t note = 0;
  uint8_t velocity = 0;
  Patch patch{};
  etl::array<int32_t, 2> feedbackBuffer{};
  etl::array<OperatorState, OperatorCount> operators{};
  etl::array<int16_t, RenderBlockSampleCount> blockBuffer{};
  uint8_t blockSamplesAvailable = 0;
  PitchEnvState pitchEnv{};
  LfoState lfo{};
  // Combined pitch modulation (PEG + LFO) for the current block, as a signed
  // Q24 log-frequency offset. Computed per block; consumed by the operator
  // frequency path (see step 5).
  int32_t pitchModQ24 = 0;
  // AudioVoice command pitch, as a signed Q24 log-frequency offset.
  int32_t pitchCommandQ24 = 0;
};

void InitializeEngine();
void StartVoice(VoiceState &voice, const Patch &patch, uint8_t note,
                uint8_t velocity = DefaultVelocity);
void SetVoiceNote(VoiceState &voice, uint8_t note);
void SetVoicePitchMultiplier(VoiceState &voice, float multiplier);
void ReleaseVoice(VoiceState &voice);
void StopVoice(VoiceState &voice);
bool RenderVoice(VoiceState &voice, int16_t *buffer, size_t sampleCount);

} // namespace fm6

#endif
