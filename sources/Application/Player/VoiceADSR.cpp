/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "VoiceADSR.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int kEnvelopeControlBlockSamples = 32;
constexpr int kEnvelopeTableSize = 128;
constexpr float kEnvelopeEpsilon = 1.0e-6f;
constexpr float kReleaseIdleThreshold = 1.0f / 65536.0f;

// Fraction of the remaining distance closed after one control block.
static constexpr float kDecayReleasePull[kEnvelopeTableSize] = {
    1.0f,         0.6272406f,   0.3506184f,   0.2501058f,   0.1888692f,
    0.1516571f,   0.1243961f,   0.1054329f,   0.09028761f,  0.07894468f,
    0.06942796f,  0.06195813f,  0.05548989f,  0.05024423f,  0.04560154f,
    0.04174415f,  0.03827513f,  0.03515854f,  0.03235895f,  0.02997229f,
    0.02780115f,  0.02582638f,  0.02411354f,  0.02246666f,  0.02103034f,
    0.01965416f,  0.01844702f,  0.01729256f,  0.01627409f,  0.01530083f,
    0.0144073f,   0.0135856f,   0.01282869f,  0.01210908f,  0.01144691f,
    0.0108194f,   0.01024191f,  0.009695647f, 0.009180239f, 0.008705877f,
    0.008258335f, 0.007836734f, 0.007440029f, 0.007067061f, 0.006710091f,
    0.006381549f, 0.00606232f,  0.005768687f, 0.005489068f, 0.005223426f,
    0.004971551f, 0.004733101f, 0.004507636f, 0.004294649f, 0.004093591f,
    0.003901681f, 0.003718934f, 0.003545246f, 0.003380428f, 0.003224223f,
    0.003076327f, 0.002935154f, 0.002800681f, 0.002672809f, 0.002551383f,
    0.002435348f, 0.002325487f, 0.002220818f, 0.002120599f, 0.002025465f,
    0.001934699f, 0.001848252f, 0.001765581f, 0.001686689f, 0.001611907f,
    0.001540378f, 0.001472099f, 0.001406745f, 0.001344587f, 0.001285294f,
    0.001228602f, 0.001174698f, 0.001122947f, 0.001073724f, 0.001026649f,
    0.00098171f,  0.000938751f, 0.000897641f, 0.000858485f, 0.000821037f,
    0.0007852f,   0.000751051f, 0.000718327f, 0.000687103f, 0.000657227f,
    0.000628695f, 0.000601383f, 0.000575337f, 0.000550399f, 0.000526568f,
    0.000503758f, 0.000481966f, 0.000461118f, 0.00044118f,  0.00042212f,
    0.000403904f, 0.00038646f,  0.000369785f, 0.000353837f, 0.000338575f,
    0.000323981f, 0.000310023f, 0.000296681f, 0.000283902f, 0.000271676f,
    0.000259989f, 0.000248808f, 0.000238105f, 0.00022787f,  0.000218072f,
    0.000208703f, 0.000199736f, 0.000191153f, 0.000182946f, 0.000175086f,
    0.000167573f, 0.000160379f, 0.0f};

// Envelope amount added per sample while attacking.
static constexpr float kAttackStep[kEnvelopeTableSize] = {
    0.03125f,       0.0086399438f,  0.0049207014f,  0.0033942978f,
    0.0025635633f,  0.002041582f,   0.0016834881f,  0.0014227856f,
    0.0012246654f,  0.0010691406f,  0.00094391551f, 0.00084101109f,
    0.00075502323f, 0.00068216166f, 0.00061968928f, 0.00056558012f,
    0.00051830246f, 0.00047667677f, 0.00043977989f, 0.00040687907f,
    0.00037738518f, 0.00035081919f, 0.0003267876f,  0.00030496414f,
    0.00028507596f, 0.00026689323f, 0.00025022084f, 0.00023489216f,
    0.00022076399f, 0.00020771264f, 0.00019563059f, 0.00018442408f,
    0.00017401084f, 0.00016431845f, 0.00015528292f, 0.00014684744f,
    0.00013896145f, 0.00013157977f, 0.00012466194f, 0.00011817156f,
    0.00011207585f, 0.00010634519f, 0.00010095273f, 9.5874115e-05f,
    9.1087146e-05f, 8.6571599e-05f, 8.2308965e-05f, 7.8282348e-05f,
    7.4476193e-05f, 7.0876209e-05f, 6.7469271e-05f, 6.4243257e-05f,
    6.1186962e-05f, 5.8290025e-05f, 5.5542871e-05f, 5.2936593e-05f,
    5.0462928e-05f, 4.8114191e-05f, 4.5883237e-05f, 4.3763386e-05f,
    4.1748412e-05f, 3.9832536e-05f, 3.8010287e-05f, 3.62766e-05f,
    3.4626704e-05f, 3.3056131e-05f, 3.1560688e-05f, 3.0136434e-05f,
    2.8779687e-05f, 2.7486938e-05f, 2.6254944e-05f, 2.508059e-05f,
    2.3960965e-05f, 2.289335e-05f,  2.1875137e-05f, 2.0903884e-05f,
    1.9977291e-05f, 1.9093146e-05f, 1.8249411e-05f, 1.7444108e-05f,
    1.6675403e-05f, 1.594155e-05f,  1.5240861e-05f, 1.4571764e-05f,
    1.3932789e-05f, 1.3322497e-05f, 1.2739547e-05f, 1.2182674e-05f,
    1.1650642e-05f, 1.1142329e-05f, 1.06566e-05f,   1.0192438e-05f,
    9.7488664e-06f, 9.3249109e-06f, 8.9196837e-06f, 8.5323554e-06f,
    8.1620819e-06f, 7.8081066e-06f, 7.4697018e-06f, 7.14614e-06f,
    6.8367662e-06f, 6.5409549e-06f, 6.2580948e-06f, 5.9875892e-06f,
    5.7288998e-06f, 5.4815027e-06f, 5.2448886e-06f, 5.0185772e-06f,
    4.8021175e-06f, 4.5950728e-06f, 4.3970358e-06f, 4.2075844e-06f,
    4.0263694e-06f, 3.8530125e-06f, 3.6871643e-06f, 3.5285047e-06f,
    3.3767137e-06f, 3.2314856e-06f, 3.0925439e-06f, 2.9596122e-06f,
    2.8324139e-06f, 2.7107162e-06f, 2.5942718e-06f, 2.4828478e-06f,
    2.3762404e-06f, 2.274217e-06f,  2.1766027e-06f, 2.083194e-06f};
} // namespace

VoiceADSR::VoiceADSR() { Reset(); }

void VoiceADSR::Reset() {
  attack_ = 0;
  decay_ = 0;
  sustain_ = 0xFF;
  release_ = 0;
  state_ = State::Idle;
  value_ = 0.0f;
  sustainLevel_ = 1.0f;
  attackStep_ = 1.0f;
  decayPull_ = 1.0f;
  releasePull_ = 1.0f;
  resetSegment();
}

void VoiceADSR::SetParameters(int attack, int decay, int sustain, int release) {
  attack_ = std::clamp(attack, 0, 0xFF);
  decay_ = std::clamp(decay, 0, 0xFF);
  sustain_ = std::clamp(sustain, 0, 0xFF);
  release_ = std::clamp(release, 0, 0xFF);
  sustainLevel_ = sustain_ / 255.0f;
  attackStep_ = attackStep(attack_);
  decayPull_ = decayReleasePull(decay_);
  releasePull_ = decayReleasePull(release_);
  updateStageSlope();
}

void VoiceADSR::NoteOn() {
  value_ = 0.0f;
  resetSegment();
  state_ = State::Attack;
  updateStageSlope();
}

void VoiceADSR::NoteOff() { enterRelease(); }

void VoiceADSR::Kill() {
  value_ = 0.0f;
  state_ = State::Idle;
  resetSegment();
}

float VoiceADSR::Advance() {
  const LinearSegment segment = AdvanceBlock(1);
  return segment.samples > 0 ? segment.firstValue : value_;
}

VoiceADSR::LinearSegment VoiceADSR::AdvanceBlock(int samples) {
  if (samples <= 0) {
    return {0, value_, 0.0f};
  }

  while (true) {
    switch (state_) {
    case State::Idle:
      return {samples, 0.0f, 0.0f};
    case State::Attack: {
      if (attackStep_ <= 0.0f) {
        value_ = 1.0f;
        enterDecay();
        continue;
      }

      const int fullStepSamples =
          static_cast<int>((1.0f - value_) / attackStep_);
      if (fullStepSamples <= 0) {
        value_ = 1.0f;
        enterDecay();
        return {1, 1.0f, 0.0f};
      }

      const int blockSamples = std::min(samples, fullStepSamples);
      const float firstValue = value_ + attackStep_;
      value_ += attackStep_ * static_cast<float>(blockSamples);
      if (value_ >= 1.0f) {
        value_ = 1.0f;
        enterDecay();
      }
      return {blockSamples, firstValue, attackStep_};
    }
    case State::Decay:
      if (segmentSamplesRemaining_ <= 0 && decayPull_ > 0.0f &&
          std::abs(value_ - sustainLevel_) <= kEnvelopeEpsilon) {
        enterSustain();
        continue;
      }
      return advanceSegmentBlock(samples, sustainLevel_, decayPull_);
    case State::Sustain:
      value_ = sustainLevel_;
      return {samples, value_, 0.0f};
    case State::Release:
      if (segmentSamplesRemaining_ <= 0 && releasePull_ > 0.0f &&
          value_ <= kReleaseIdleThreshold) {
        Kill();
        continue;
      }
      return advanceSegmentBlock(samples, 0.0f, releasePull_);
    }
  }
}

void VoiceADSR::AdvanceSilent(int samples) {
  while (samples > 0) {
    const LinearSegment segment = AdvanceBlock(samples);
    if (segment.samples <= 0) {
      break;
    }
    samples -= segment.samples;
  }
}

int VoiceADSR::tableIndex(int value) {
  value = std::clamp(value, 0, 0xFF);
  if (value <= 0) {
    return 0;
  }
  if (value >= 0xFF) {
    return kEnvelopeTableSize - 1;
  }

  return 1 + ((value - 1) * (kEnvelopeTableSize - 2)) / 0xFE;
}

float VoiceADSR::attackStep(int value) {
  return kAttackStep[tableIndex(value)];
}

float VoiceADSR::decayReleasePull(int value) {
  return kDecayReleasePull[tableIndex(value)];
}

void VoiceADSR::resetSegment() {
  segmentStep_ = 0.0f;
  segmentSamplesRemaining_ = 0;
}

VoiceADSR::LinearSegment
VoiceADSR::advanceSegmentBlock(int samples, float target, float pull) {
  if (segmentSamplesRemaining_ <= 0) {
    const float next = value_ + (target - value_) * pull;
    segmentStep_ =
        (next - value_) / static_cast<float>(kEnvelopeControlBlockSamples);
    segmentSamplesRemaining_ = kEnvelopeControlBlockSamples;
  }

  const int blockSamples = std::min(samples, segmentSamplesRemaining_);
  const float firstValue = value_ + segmentStep_;
  value_ += segmentStep_ * static_cast<float>(blockSamples);
  segmentSamplesRemaining_ -= blockSamples;
  return {blockSamples, firstValue, segmentStep_};
}

void VoiceADSR::updateStageSlope() {
  switch (state_) {
  case State::Idle:
    return;
  case State::Attack: {
    if (value_ >= 1.0f - kEnvelopeEpsilon) {
      value_ = 1.0f;
      enterDecay();
      return;
    }

    if (attack_ <= 0) {
      value_ = 1.0f;
      enterDecay();
      return;
    }

    attackStep_ = attackStep(attack_);
    return;
  }
  case State::Decay:
    enterDecay();
    return;
  case State::Sustain:
    value_ = sustainLevel_;
    return;
  case State::Release:
    enterRelease();
    return;
  }
}

void VoiceADSR::enterDecay() {
  const float delta = sustainLevel_ - value_;
  if (std::abs(delta) <= kEnvelopeEpsilon || decay_ <= 0) {
    enterSustain();
    return;
  }

  resetSegment();
  state_ = State::Decay;
}

void VoiceADSR::enterSustain() {
  value_ = sustainLevel_;
  state_ = State::Sustain;
  resetSegment();
}

void VoiceADSR::enterRelease() {
  if (state_ == State::Idle) {
    return;
  }

  if (release_ <= 0 || value_ <= 0.0f) {
    Kill();
    return;
  }

  resetSegment();
  state_ = State::Release;
}
