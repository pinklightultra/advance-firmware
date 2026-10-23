/*
 * SPDX-License-Identifier: Apache-2.0 AND GPL-3.0-only
 *
 * Copyright 2012 Google Inc.
 * Copyright 2013 Google Inc.
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * This is a port of the "msfa" DX7 emulation engine, taken
 * from the libmsfa project (https://github.com/maks/libmsfa)
 * which is made available under the Apache License 2.0
 *
 * Credits & thanks
 * Original Synth engine: Raph Levien and the msfa team
 * Updates to the Synth engine: The Dexed team
 * CMake updates to original msfa code: Maksim Lin
 *
 * Contains code adapted from libmsfa/msfa under Apache-2.0.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Original notices: sources/Application/Instruments/AdvanceFM6/NOTICE.
 * License text: LICENSES/Apache-2.0.txt at the repository root.
 * Existing license grants are preserved. See LICENSING.md.
 */

#include "FM6Voice.h"

#include "Externals/etl/include/etl/array.h"
#include "config/MemorySections.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace fm6 {
namespace {

static constexpr uint32_t NoteCount = 128U;
static constexpr uint32_t Dx7LgBlockSamples = 6U;
static constexpr uint32_t Dx7BlockSamples = 1U << Dx7LgBlockSamples;
static constexpr uint32_t Dx7SinLgSampleCount = 10U;
static constexpr uint32_t Dx7SinSampleCount = 1U << Dx7SinLgSampleCount;
static constexpr uint32_t Dx7Exp2LgSampleCount = 10U;
static constexpr uint32_t Dx7Exp2SampleCount = 1U << Dx7Exp2LgSampleCount;
static constexpr uint32_t Dx7FreqLutLgSampleCount = 10U;
static constexpr uint32_t Dx7FreqLutSampleCount = 1U << Dx7FreqLutLgSampleCount;
static constexpr int32_t Dx7FreqLutMaxLogFreqInt = 20;
static constexpr int32_t Dx7FreqLutSampleShift =
    24 - static_cast<int32_t>(Dx7FreqLutLgSampleCount);
static constexpr uint32_t MixShift = 8U;
static constexpr uint32_t Q24ToS16Shift = 9U;
static constexpr uint8_t VoiceLevel = 48U;
static constexpr uint8_t MasterLevel = 255U;
static constexpr uint32_t SampleRate = 44100U;

enum Dx7BusFlags : uint8_t {
  Dx7OutBusAdd = 1U << 2,
  Dx7FeedbackIn = 1U << 6,
  Dx7FeedbackOut = 1U << 7,
};

struct Dx7OperatorParams {
  etl::array<int32_t, 2> gain{};
  uint32_t frequency = 0;
  uint32_t phase = 0;
};

struct Dx7Algorithm {
  etl::array<uint8_t, OperatorCount> operators{};
};

#include "FM6VoiceLuts.h"

constexpr etl::array<int32_t, 20> dx7LevelLut = {0,  5,  9,  13, 17, 20, 23,
                                                 25, 27, 29, 31, 33, 35, 37,
                                                 39, 41, 42, 43, 45, 46};

constexpr etl::array<uint8_t, 64> dx7VelocityLut = {
    0,   70,  86,  97,  106, 114, 121, 126, 132, 138, 142, 148, 152,
    156, 160, 163, 166, 170, 173, 174, 178, 181, 184, 186, 189, 190,
    194, 196, 198, 200, 202, 205, 206, 209, 211, 214, 216, 218, 220,
    222, 224, 225, 227, 229, 230, 232, 233, 235, 237, 238, 240, 241,
    242, 243, 244, 246, 246, 248, 249, 250, 251, 252, 253, 254};

constexpr etl::array<uint8_t, 33> dx7ExpScaleLut = {
    0,  1,  2,  3,  4,  5,  6,   7,   8,   9,   11,  14,  16,  19,  23,  27, 33,
    39, 47, 56, 66, 80, 94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 250};

constexpr etl::array<Dx7Algorithm, 32> dx7Algorithms = {{
    {{0xc1, 0x11, 0x11, 0x14, 0x01, 0x14}},
    {{0x01, 0x11, 0x11, 0x14, 0xc1, 0x14}},
    {{0xc1, 0x11, 0x14, 0x01, 0x11, 0x14}},
    {{0x41, 0x11, 0x94, 0x01, 0x11, 0x14}},
    {{0xc1, 0x14, 0x01, 0x14, 0x01, 0x14}},
    {{0x41, 0x94, 0x01, 0x14, 0x01, 0x14}},
    {{0xc1, 0x11, 0x05, 0x14, 0x01, 0x14}},
    {{0x01, 0x11, 0xc5, 0x14, 0x01, 0x14}},
    {{0x01, 0x11, 0x05, 0x14, 0xc1, 0x14}},
    {{0x01, 0x05, 0x14, 0xc1, 0x11, 0x14}},
    {{0xc1, 0x05, 0x14, 0x01, 0x11, 0x14}},
    {{0x01, 0x05, 0x05, 0x14, 0xc1, 0x14}},
    {{0xc1, 0x05, 0x05, 0x14, 0x01, 0x14}},
    {{0xc1, 0x05, 0x11, 0x14, 0x01, 0x14}},
    {{0x01, 0x05, 0x11, 0x14, 0xc1, 0x14}},
    {{0xc1, 0x11, 0x02, 0x25, 0x05, 0x14}},
    {{0x01, 0x11, 0x02, 0x25, 0xc5, 0x14}},
    {{0x01, 0x11, 0x11, 0xc5, 0x05, 0x14}},
    {{0xc1, 0x14, 0x14, 0x01, 0x11, 0x14}},
    {{0x01, 0x05, 0x14, 0xc1, 0x14, 0x14}},
    {{0x01, 0x14, 0x14, 0xc1, 0x14, 0x14}},
    {{0xc1, 0x14, 0x14, 0x14, 0x01, 0x14}},
    {{0xc1, 0x14, 0x14, 0x01, 0x14, 0x04}},
    {{0xc1, 0x14, 0x14, 0x14, 0x04, 0x04}},
    {{0xc1, 0x14, 0x14, 0x04, 0x04, 0x04}},
    {{0xc1, 0x05, 0x14, 0x01, 0x14, 0x04}},
    {{0x01, 0x05, 0x14, 0xc1, 0x14, 0x04}},
    {{0x04, 0xc1, 0x11, 0x14, 0x01, 0x14}},
    {{0xc1, 0x14, 0x01, 0x14, 0x04, 0x04}},
    {{0x04, 0xc1, 0x11, 0x14, 0x04, 0x04}},
    {{0xc1, 0x14, 0x04, 0x04, 0x04, 0x04}},
    {{0xc4, 0x04, 0x04, 0x04, 0x04, 0x04}},
}};

int16_t clampS16(int32_t value) {
  if (value > 32767) {
    return 32767;
  }
  if (value < -32768) {
    return -32768;
  }
  return static_cast<int16_t>(value);
}

int32_t scaleOutputLevel(int32_t outputLevel) {
  if (outputLevel >= 20) {
    return 28 + outputLevel;
  }

  return dx7LevelLut[static_cast<size_t>(outputLevel)];
}

int32_t scaleVelocity(int32_t velocity, int32_t sensitivity) {
  const int32_t clampedVelocity = std::clamp<int32_t>(velocity, 0, 127);
  const int32_t velocityValue =
      dx7VelocityLut[static_cast<size_t>(clampedVelocity >> 1)] - 239;
  return ((sensitivity * velocityValue + 7) >> 3) << 4;
}

uint8_t scaleRate(uint8_t note, uint8_t sensitivity) {
  int32_t x = static_cast<int32_t>(note) / 3 - 7;
  x = std::clamp<int32_t>(x, 0, 31);
  return static_cast<uint8_t>((static_cast<int32_t>(sensitivity) * x) >> 3);
}

int32_t scaleCurve(int32_t group, int32_t depth, int32_t curve) {
  int32_t scale = 0;
  if (curve == 0 || curve == 3) {
    scale = (group * depth * 329) >> 12;
  } else {
    const int32_t rawExp =
        dx7ExpScaleLut[static_cast<size_t>(std::min<int32_t>(group, 32))];
    scale = (rawExp * depth * 329) >> 15;
  }

  if (curve < 2) {
    scale = -scale;
  }
  return scale;
}

int32_t scaleLevel(uint8_t note, const OperatorPatch &patch) {
  const int32_t offset =
      static_cast<int32_t>(note) - static_cast<int32_t>(patch.breakPoint) - 17;
  if (offset >= 0) {
    return scaleCurve(offset / 3, patch.rightDepth, patch.rightCurve);
  }
  return scaleCurve((-offset) / 3, patch.leftDepth, patch.leftCurve);
}

void advanceEnvelope(Dx7EnvelopeState &envelope, const OperatorPatch &patch,
                     uint8_t newStageIndex) {
  envelope.stageIndex = newStageIndex;
  if (newStageIndex >= 4U) {
    return;
  }

  int32_t actualLevel = scaleOutputLevel(patch.levels[newStageIndex]) >> 1;
  actualLevel = (actualLevel << 6) + envelope.outputLevel - 4256;
  if (actualLevel < 16) {
    actualLevel = 16;
  }

  envelope.targetLevelQ24 = actualLevel << 16;
  envelope.rising = envelope.targetLevelQ24 > envelope.levelQ24;

  int32_t qrate = (static_cast<int32_t>(patch.rates[newStageIndex]) * 41) >> 6;
  qrate += envelope.rateScaling;
  qrate = std::min<int32_t>(qrate, 63);

  envelope.incrementQ24 = static_cast<uint32_t>(
      (4 + (qrate & 3)) << (2 + Dx7LgBlockSamples + (qrate >> 2)));
}

void initializeEnvelope(Dx7EnvelopeState &envelope, const OperatorPatch &patch,
                        uint8_t note, uint8_t velocity) {
  int32_t outputLevel = scaleOutputLevel(patch.outputLevel);
  outputLevel += scaleLevel(note, patch);
  outputLevel = std::min<int32_t>(outputLevel, 127);
  outputLevel <<= 5;
  outputLevel += scaleVelocity(velocity, patch.keyVelocitySensitivity);
  outputLevel = std::max<int32_t>(outputLevel, 0);

  envelope = {};
  envelope.outputLevel = outputLevel;
  envelope.rateScaling = scaleRate(note, patch.rateScaling);
  envelope.keyDown = true;
  advanceEnvelope(envelope, patch, 0U);
}

void setEnvelopeKeyDown(Dx7EnvelopeState &envelope, const OperatorPatch &patch,
                        bool keyDown) {
  if (envelope.keyDown == keyDown) {
    return;
  }

  envelope.keyDown = keyDown;
  advanceEnvelope(envelope, patch, keyDown ? 0U : 3U);
}

int32_t nextEnvelopeSample(Dx7EnvelopeState &envelope,
                           const OperatorPatch &patch) {
  if (envelope.stageIndex < 3U ||
      (envelope.stageIndex < 4U && !envelope.keyDown)) {
    if (envelope.rising) {
      static constexpr int32_t JumpTarget = 1716 << 16;
      if (envelope.levelQ24 < JumpTarget) {
        envelope.levelQ24 = JumpTarget;
      }

      envelope.levelQ24 += (((17 << 24) - envelope.levelQ24) >> 24) *
                           static_cast<int32_t>(envelope.incrementQ24);
      if (envelope.levelQ24 >= envelope.targetLevelQ24) {
        envelope.levelQ24 = envelope.targetLevelQ24;
        advanceEnvelope(envelope, patch,
                        static_cast<uint8_t>(envelope.stageIndex + 1U));
      }
    } else {
      envelope.levelQ24 -= static_cast<int32_t>(envelope.incrementQ24);
      if (envelope.levelQ24 <= envelope.targetLevelQ24) {
        envelope.levelQ24 = envelope.targetLevelQ24;
        advanceEnvelope(envelope, patch,
                        static_cast<uint8_t>(envelope.stageIndex + 1U));
      }
    }
  }

  return envelope.levelQ24;
}

bool isEnvelopeFinished(const Dx7EnvelopeState &envelope) {
  return envelope.stageIndex >= 4U;
}

// Log-frequency (Q24, 1<<24 == one octave) of a MIDI note, referenced to
// A4 = 440 Hz. Matches libmsfa midinote_to_logfreq.
int32_t midiNoteToLogFreq(int32_t note) {
  constexpr int32_t base = 50857777; // (1<<24) * (log2(440) - 69/12)
  constexpr int32_t step = (1 << 24) / 12;
  return base + step * note;
}

// Resolves a Q24 log-frequency into a phase increment (2^24 per cycle, matching
// the FM core's phase accumulator). Ported from libmsfa freqlut.cc: an octave
// table with linear interpolation, plus a variable shift for the integer
// octave. The shift is clamped so a pathological pitch (well above Nyquist or
// far below audibility) can never trigger undefined shift behaviour.
uint32_t freqLutLookup(int32_t logFreq) {
  const int32_t ix = (logFreq & 0xffffff) >> Dx7FreqLutSampleShift;
  const int32_t y0 = freqLut[static_cast<size_t>(ix)];
  const int32_t y1 = freqLut[static_cast<size_t>(ix + 1)];
  const int32_t lowBits = logFreq & ((1 << Dx7FreqLutSampleShift) - 1);
  const int32_t y =
      y0 + static_cast<int32_t>((static_cast<int64_t>(y1 - y0) * lowBits) >>
                                Dx7FreqLutSampleShift);
  const int32_t hiBits = logFreq >> 24;
  const int32_t shift =
      std::clamp<int32_t>(Dx7FreqLutMaxLogFreqInt - hiBits, 0, 31);
  return static_cast<uint32_t>(y >> shift);
}

// Base log-frequency (Q24) for an operator at a given note, before per-block
// pitch modulation. Ratio operators track the (transposed) note; fixed
// operators use their absolute frequency. The ratio/detune/fixed terms were
// precomputed into logFreqOffsetQ24 at patch-conversion time.
int32_t operatorBaseLogFreq(uint8_t note, const Patch &patch,
                            const OperatorPatch &operatorPatch) {
  if (operatorPatch.fixedFrequency) {
    return operatorPatch.logFreqOffsetQ24;
  }
  const int32_t transposedNote = std::clamp<int32_t>(
      static_cast<int32_t>(note) + static_cast<int32_t>(patch.transpose), 0,
      static_cast<int32_t>(NoteCount - 1U));
  return midiNoteToLogFreq(transposedNote) + operatorPatch.logFreqOffsetQ24;
}

int32_t sinLookup(uint32_t phase) {
  const int32_t shift = 24 - static_cast<int32_t>(Dx7SinLgSampleCount);
  const int32_t lowBits = static_cast<int32_t>(phase & ((1U << shift) - 1U));
  const int32_t phaseIndex = static_cast<int32_t>(
      (phase >> (shift - 1)) & ((Dx7SinSampleCount - 1U) << 1));
  const int32_t delta = sinLut[static_cast<size_t>(phaseIndex)];
  const int32_t y0 = sinLut[static_cast<size_t>(phaseIndex + 1)];
  return y0 +
         static_cast<int32_t>((static_cast<int64_t>(delta) * lowBits) >> shift);
}

int32_t exp2Lookup(int32_t x) {
  const int32_t shift = 24 - static_cast<int32_t>(Dx7Exp2LgSampleCount);
  const int32_t lowBits = x & ((1 << shift) - 1);
  const int32_t xIndex = (x >> (shift - 1)) &
                         static_cast<int32_t>(((Dx7Exp2SampleCount - 1U) << 1));
  const int32_t delta = exp2Lut[static_cast<size_t>(xIndex)];
  const int32_t y0 = exp2Lut[static_cast<size_t>(xIndex + 1)];
  const int32_t y = y0 + static_cast<int32_t>(
                             (static_cast<int64_t>(delta) * lowBits) >> shift);
  return y >> (6 - (x >> 24));
}

int32_t feedbackShift(const Patch &patch) {
  if (patch.feedback == 0U) {
    return 16;
  }
  return 8 - static_cast<int32_t>(patch.feedback);
}

// ---------------------------------------------------------------------------
// Pitch envelope generator (ported from libmsfa pitchenv.cc). One PEG per
// voice, ticked once per render block; output is a signed Q24 log-frequency
// offset shared by every operator.
// ---------------------------------------------------------------------------

// unit_ = N * (1 << 24) / (21.3 * sampleRate) + 0.5, evaluated for the render
// block size and sample rate. Governs how far the envelope advances per block.
static constexpr int32_t PitchEnvUnit = static_cast<int32_t>(
    static_cast<double>(Dx7BlockSamples) * static_cast<double>(1 << 24) /
        (21.3 * static_cast<double>(SampleRate)) +
    0.5);

constexpr etl::array<uint8_t, 100> pitchEnvRateLut = {
    1,   2,   3,   3,   4,   4,   5,   5,   6,   6,   7,   7,   8,   8,   9,
    9,   10,  10,  11,  11,  12,  12,  13,  13,  14,  14,  15,  16,  16,  17,
    18,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  30,  31,  33,
    34,  36,  37,  38,  39,  41,  42,  44,  46,  47,  49,  51,  53,  54,  56,
    58,  60,  62,  64,  66,  68,  70,  72,  74,  76,  79,  82,  85,  88,  91,
    94,  98,  102, 106, 110, 115, 120, 125, 130, 135, 141, 147, 153, 159, 165,
    171, 178, 185, 193, 202, 211, 232, 243, 254, 255};

constexpr etl::array<int8_t, 100> pitchEnvLevelLut = {
    -128, -116, -104, -95, -85, -76, -68, -61, -56, -52, -49, -46, -43,
    -41,  -39,  -37,  -35, -33, -32, -31, -30, -29, -28, -27, -26, -25,
    -24,  -23,  -22,  -21, -20, -19, -18, -17, -16, -15, -14, -13, -12,
    -11,  -10,  -9,   -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,  0,   1,
    2,    3,    4,    5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,   16,   17,   18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
    28,   29,   30,   31,  32,  33,  34,  35,  38,  40,  43,  46,  49,
    53,   58,   65,   73,  82,  92,  103, 115, 127};

void pitchEnvAdvance(PitchEnvState &env, const PitchEgPatch &patch,
                     uint8_t newStageIndex) {
  env.stageIndex = newStageIndex;
  if (newStageIndex >= 4U) {
    return;
  }
  const uint8_t newLevel = patch.levels[newStageIndex];
  env.targetLevel = static_cast<int32_t>(pitchEnvLevelLut[newLevel]) << 19;
  env.rising = env.targetLevel > env.level;
  env.increment =
      static_cast<int32_t>(pitchEnvRateLut[patch.rates[newStageIndex]]) *
      PitchEnvUnit;
}

void pitchEnvInit(PitchEnvState &env, const PitchEgPatch &patch) {
  env = {};
  env.level = static_cast<int32_t>(pitchEnvLevelLut[patch.levels[3]]) << 19;
  env.keyDown = true;
  pitchEnvAdvance(env, patch, 0U);
}

int32_t pitchEnvNextSample(PitchEnvState &env, const PitchEgPatch &patch) {
  if (env.stageIndex < 3U || (env.stageIndex < 4U && !env.keyDown)) {
    if (env.rising) {
      env.level += env.increment;
      if (env.level >= env.targetLevel) {
        env.level = env.targetLevel;
        pitchEnvAdvance(env, patch, static_cast<uint8_t>(env.stageIndex + 1U));
      }
    } else {
      env.level -= env.increment;
      if (env.level <= env.targetLevel) {
        env.level = env.targetLevel;
        pitchEnvAdvance(env, patch, static_cast<uint8_t>(env.stageIndex + 1U));
      }
    }
  }
  return env.level;
}

void pitchEnvKeyDown(PitchEnvState &env, const PitchEgPatch &patch,
                     bool keyDown) {
  if (env.keyDown != keyDown) {
    env.keyDown = keyDown;
    pitchEnvAdvance(env, patch, keyDown ? 0U : 3U);
  }
}

// ---------------------------------------------------------------------------
// Low-frequency oscillator (ported from libmsfa lfo.cc). One LFO per voice.
// getsample() yields the waveform value (0..1<<24), getdelay() the delay-ramp
// envelope; both are ticked once per render block.
// ---------------------------------------------------------------------------

// unit_ = N * 25190424 / sampleRate + 0.5 (1<<32 / 15.5s / 11 per the source).
static constexpr uint32_t LfoUnit =
    static_cast<uint32_t>(static_cast<double>(Dx7BlockSamples) * 25190424.0 /
                              static_cast<double>(SampleRate) +
                          0.5);

constexpr etl::array<uint8_t, 8> pitchModSensLut = {0,  10, 20,  33,
                                                    55, 92, 153, 255};

void lfoReset(LfoState &lfo, const LfoPatch &patch) {
  const int32_t rate = static_cast<int32_t>(patch.speed); // 0..99
  int32_t sr = rate == 0 ? 1 : (165 * rate) >> 6;
  sr *= sr < 160 ? 11 : (11 + ((sr - 160) >> 4));
  lfo.delta = LfoUnit * static_cast<uint32_t>(sr);

  int32_t a = 99 - static_cast<int32_t>(patch.delay); // LFO delay
  if (a == 99) {
    lfo.delayIncrement = ~0U;
    lfo.delayIncrement2 = ~0U;
  } else {
    a = (16 + (a & 15)) << (1 + (a >> 4));
    lfo.delayIncrement = LfoUnit * static_cast<uint32_t>(a);
    a &= 0xff80;
    a = std::max<int32_t>(0x80, a);
    lfo.delayIncrement2 = LfoUnit * static_cast<uint32_t>(a);
  }
  lfo.waveform = patch.waveform;
  lfo.sync = patch.sync;
}

int32_t lfoGetSample(LfoState &lfo) {
  lfo.phase += lfo.delta;
  int32_t x;
  switch (lfo.waveform) {
  case 0: // triangle
    x = static_cast<int32_t>(lfo.phase >> 7);
    x ^= -static_cast<int32_t>(lfo.phase >> 31);
    x &= (1 << 24) - 1;
    return x;
  case 1: // sawtooth down
    return static_cast<int32_t>((~lfo.phase ^ (1U << 31)) >> 8);
  case 2: // sawtooth up
    return static_cast<int32_t>((lfo.phase ^ (1U << 31)) >> 8);
  case 3: // square
    return static_cast<int32_t>(((~lfo.phase) >> 7) & (1U << 24));
  case 4: // sine
    return (1 << 23) + (sinLookup(lfo.phase >> 8) >> 1);
  case 5: // sample & hold
    if (lfo.phase < lfo.delta) {
      lfo.randState = (lfo.randState * 179 + 17) & 0xff;
    }
    x = lfo.randState ^ 0x80;
    return (x + 1) << 16;
  }
  return 1 << 23;
}

int32_t lfoGetDelay(LfoState &lfo) {
  const uint32_t delta =
      lfo.delayState < (1U << 31) ? lfo.delayIncrement : lfo.delayIncrement2;
  const uint32_t d = lfo.delayState + delta;
  if (d < lfo.delayIncrement) {
    return 1 << 24;
  }
  lfo.delayState = d;
  if (d < (1U << 31)) {
    return 0;
  }
  return static_cast<int32_t>((d >> 7) & ((1 << 24) - 1));
}

void lfoKeyDown(LfoState &lfo) {
  if (lfo.sync) {
    lfo.phase = (1U << 31) - 1;
  }
  lfo.delayState = 0;
}

// Combines the pitch EG and LFO into the per-block pitch modulation offset
// (signed Q24 log-frequency), mirroring libmsfa Dx7Note::compute. The LFO
// contributes vibrato scaled by pitch-mod depth, the delay ramp, and pitch-mod
// sensitivity; pitch bend / mod wheel controllers are intentionally omitted.
int32_t computePitchMod(VoiceState &voice) {
  const int32_t lfoValue = lfoGetSample(voice.lfo);
  const int32_t lfoDelay = lfoGetDelay(voice.lfo);
  int32_t pitchMod = pitchEnvNextSample(voice.pitchEnv, voice.patch.pitchEg);

  const int32_t pitchModDepth =
      (static_cast<int32_t>(voice.patch.lfo.pitchModDepth) * 165) >> 6;
  const int32_t pitchModSens = static_cast<int32_t>(
      pitchModSensLut[voice.patch.lfo.pitchModSensitivity & 7]);
  const uint32_t pmd =
      static_cast<uint32_t>(pitchModDepth) * static_cast<uint32_t>(lfoDelay);
  const int32_t sensLfo = pitchModSens * (lfoValue - (1 << 23));
  pitchMod += static_cast<int32_t>(
      (static_cast<int64_t>(pmd) * static_cast<int64_t>(sensLfo)) >> 39);
  return pitchMod;
}

bool isVoiceFinished(const VoiceState &voice) {
  for (const auto &op : voice.operators) {
    if (!isEnvelopeFinished(op.envelope)) {
      return false;
    }
  }
  return true;
}

void computeKernel(int32_t *output, const int32_t *input, uint32_t phase0,
                   uint32_t frequency, int32_t gain1, int32_t gain2, bool add) {
  int32_t gain = gain1;
  const int32_t deltaGain =
      (gain2 - gain1 + (static_cast<int32_t>(Dx7BlockSamples) >> 1)) >>
      Dx7LgBlockSamples;
  uint32_t phase = phase0;

  for (size_t i = 0; i < Dx7BlockSamples; ++i) {
    gain += deltaGain;
    const int32_t value =
        static_cast<int32_t>((static_cast<int64_t>(sinLookup(
                                  phase + static_cast<uint32_t>(input[i]))) *
                              static_cast<int64_t>(gain)) >>
                             24);
    output[i] = add ? output[i] + value : value;
    phase += frequency;
  }
}

void computeKernelPure(int32_t *output, uint32_t phase0, uint32_t frequency,
                       int32_t gain1, int32_t gain2, bool add) {
  int32_t gain = gain1;
  const int32_t deltaGain =
      (gain2 - gain1 + (static_cast<int32_t>(Dx7BlockSamples) >> 1)) >>
      Dx7LgBlockSamples;
  uint32_t phase = phase0;

  for (size_t i = 0; i < Dx7BlockSamples; ++i) {
    gain += deltaGain;
    const int32_t value = static_cast<int32_t>(
        (static_cast<int64_t>(sinLookup(phase)) * static_cast<int64_t>(gain)) >>
        24);
    output[i] = add ? output[i] + value : value;
    phase += frequency;
  }
}

void computeKernelFeedback(int32_t *output, uint32_t phase0, uint32_t frequency,
                           int32_t gain1, int32_t gain2,
                           int32_t *feedbackBuffer, int32_t feedbackAmountShift,
                           bool add) {
  int32_t gain = gain1;
  const int32_t deltaGain =
      (gain2 - gain1 + (static_cast<int32_t>(Dx7BlockSamples) >> 1)) >>
      Dx7LgBlockSamples;
  uint32_t phase = phase0;
  int32_t y0 = feedbackBuffer[0];
  int32_t y = feedbackBuffer[1];

  for (size_t i = 0; i < Dx7BlockSamples; ++i) {
    const int32_t scaledFeedback = (y0 + y) >> (feedbackAmountShift + 1);
    gain += deltaGain;
    y0 = y;
    y = sinLookup(phase + static_cast<uint32_t>(scaledFeedback));
    y = static_cast<int32_t>(
        (static_cast<int64_t>(y) * static_cast<int64_t>(gain)) >> 24);
    output[i] = add ? output[i] + y : y;
    phase += frequency;
  }

  feedbackBuffer[0] = y0;
  feedbackBuffer[1] = y;
}

void computeDx7Block(int32_t *output,
                     etl::array<Dx7OperatorParams, OperatorCount> &params,
                     uint8_t algorithm, int32_t *feedbackBuffer,
                     int32_t feedbackAmountShift) {
  static constexpr int32_t LevelThreshold = 1120;
  etl::array<int32_t, Dx7BlockSamples> busOne{};
  etl::array<int32_t, Dx7BlockSamples> busTwo{};
  etl::array<bool, 3> hasContents{true, false, false};
  const Dx7Algorithm &algorithmDef =
      dx7Algorithms[std::min<size_t>(algorithm, dx7Algorithms.size() - 1U)];

  for (size_t op = 0; op < OperatorCount; ++op) {
    const uint8_t flags = algorithmDef.operators[op];
    bool add = (flags & Dx7OutBusAdd) != 0U;
    const uint8_t inBus = (flags >> 4) & 3U;
    const uint8_t outBus = flags & 3U;
    int32_t *out =
        outBus == 0U ? output : (outBus == 1U ? busOne.data() : busTwo.data());
    auto &param = params[op];
    const int32_t gain1 = param.gain[0];
    const int32_t gain2 = param.gain[1];

    if (gain1 >= LevelThreshold || gain2 >= LevelThreshold) {
      if (!hasContents[outBus]) {
        add = false;
      }

      if (inBus == 0U || !hasContents[inBus]) {
        if ((flags & 0xc0U) == (Dx7FeedbackIn | Dx7FeedbackOut) &&
            feedbackAmountShift < 16) {
          computeKernelFeedback(out, param.phase, param.frequency, gain1, gain2,
                                feedbackBuffer, feedbackAmountShift, add);
        } else {
          computeKernelPure(out, param.phase, param.frequency, gain1, gain2,
                            add);
        }
      } else {
        computeKernel(out, inBus == 1U ? busOne.data() : busTwo.data(),
                      param.phase, param.frequency, gain1, gain2, add);
      }
      hasContents[outBus] = true;
    } else if (!add) {
      hasContents[outBus] = false;
    }

    param.phase += param.frequency << Dx7LgBlockSamples;
  }
}

} // namespace

void InitializeEngine() { InitializeBuiltInPatches(); }

void StartVoice(VoiceState &voice, const Patch &patch, uint8_t note,
                uint8_t velocity) {
  InitializeEngine();
  voice = {};
  voice.active = true;
  voice.note = note;
  voice.velocity = velocity;
  voice.patch = patch;

  for (size_t i = 0; i < OperatorCount; ++i) {
    voice.operators[i].baseLogFreq =
        operatorBaseLogFreq(note, voice.patch, voice.patch.operators[i]);
    voice.operators[i].phaseStep =
        freqLutLookup(voice.operators[i].baseLogFreq + voice.pitchModQ24 +
                      voice.pitchCommandQ24);
    initializeEnvelope(voice.operators[i].envelope, voice.patch.operators[i],
                       note, velocity);
  }

  pitchEnvInit(voice.pitchEnv, voice.patch.pitchEg);
  lfoReset(voice.lfo, voice.patch.lfo);
  lfoKeyDown(voice.lfo);
}

void SetVoiceNote(VoiceState &voice, uint8_t note) {
  if (!voice.active) {
    return;
  }

  // Legato pitch change: retune the operators while the envelopes, phases and
  // feedback state keep running, matching the SID instrument's non-retrigger
  // note starts.
  voice.note = note;
  for (size_t i = 0; i < OperatorCount; ++i) {
    voice.operators[i].baseLogFreq =
        operatorBaseLogFreq(note, voice.patch, voice.patch.operators[i]);
    voice.operators[i].phaseStep =
        freqLutLookup(voice.operators[i].baseLogFreq + voice.pitchModQ24 +
                      voice.pitchCommandQ24);
  }
}

void SetVoicePitchMultiplier(VoiceState &voice, float multiplier) {
  if (!voice.active) {
    return;
  }

  constexpr float q24PerOctave = static_cast<float>(1 << 24);
  const float positiveMultiplier = multiplier > 0.0f ? multiplier : 1.0e-6f;
  voice.pitchCommandQ24 = static_cast<int32_t>(
      std::lround(std::log2(positiveMultiplier) * q24PerOctave));
  for (size_t i = 0; i < OperatorCount; ++i) {
    voice.operators[i].phaseStep =
        freqLutLookup(voice.operators[i].baseLogFreq + voice.pitchModQ24 +
                      voice.pitchCommandQ24);
  }
}

void ReleaseVoice(VoiceState &voice) {
  if (!voice.active) {
    return;
  }

  for (size_t i = 0; i < OperatorCount; ++i) {
    setEnvelopeKeyDown(voice.operators[i].envelope, voice.patch.operators[i],
                       false);
  }
  pitchEnvKeyDown(voice.pitchEnv, voice.patch.pitchEg, false);
}

void StopVoice(VoiceState &voice) { voice = {}; }

namespace {

PICOTRACKER_FAST_CODE("fm6_render_block")
void renderOneBlock(VoiceState &voice) {
  etl::array<int32_t, Dx7BlockSamples> monoBlock{};
  etl::array<Dx7OperatorParams, OperatorCount> params{};

  // Advance the pitch EG and LFO once per block to get the shared pitch
  // modulation offset (Q24 log-frequency), then resolve each operator's phase
  // step from its note-constant base pitch plus that offset.
  voice.pitchModQ24 = computePitchMod(voice);

  for (size_t op = 0; op < OperatorCount; ++op) {
    voice.operators[op].phaseStep =
        freqLutLookup(voice.operators[op].baseLogFreq + voice.pitchModQ24 +
                      voice.pitchCommandQ24);
    params[op].phase = voice.operators[op].phase;
    params[op].frequency = voice.operators[op].phaseStep;
    params[op].gain[0] = voice.operators[op].gainQ24;
    params[op].gain[1] =
        exp2Lookup(nextEnvelopeSample(voice.operators[op].envelope,
                                      voice.patch.operators[op]) -
                   (14 * (1 << 24)));
  }

  computeDx7Block(monoBlock.data(), params, voice.patch.algorithm,
                  voice.feedbackBuffer.data(), feedbackShift(voice.patch));

  for (size_t op = 0; op < OperatorCount; ++op) {
    voice.operators[op].phase = params[op].phase;
    voice.operators[op].gainQ24 = params[op].gain[1];
  }

  for (size_t i = 0; i < Dx7BlockSamples; ++i) {
    const int32_t sample = static_cast<int32_t>(
        (static_cast<int64_t>(monoBlock[i]) * static_cast<int64_t>(VoiceLevel) *
         static_cast<int64_t>(MasterLevel)) >>
        (Q24ToS16Shift + MixShift + 8));
    voice.blockBuffer[i] = clampS16(sample);
  }
}

} // namespace

PICOTRACKER_FAST_CODE("fm6_render_voice")
bool RenderVoice(VoiceState &voice, int16_t *buffer, size_t sampleCount) {
  if (!voice.active || !buffer || sampleCount == 0U) {
    return false;
  }

  size_t outputPos = 0;
  while (outputPos < sampleCount) {
    if (voice.blockSamplesAvailable == 0U) {
      renderOneBlock(voice);
      voice.blockSamplesAvailable =
          static_cast<uint8_t>(RenderBlockSampleCount);
    }

    const size_t readPos = static_cast<size_t>(RenderBlockSampleCount) -
                           static_cast<size_t>(voice.blockSamplesAvailable);
    const size_t toCopy =
        std::min(static_cast<size_t>(voice.blockSamplesAvailable),
                 sampleCount - outputPos);
    for (size_t i = 0; i < toCopy; ++i) {
      buffer[outputPos + i] = voice.blockBuffer[readPos + i];
    }
    voice.blockSamplesAvailable -= static_cast<uint8_t>(toCopy);
    outputPos += toCopy;
  }

  if (voice.blockSamplesAvailable == 0U && isVoiceFinished(voice)) {
    voice = {};
  }

  return true;
}

} // namespace fm6
