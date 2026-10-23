/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "LFOProcessor.h"
#include "Services/Audio/Audio.h"
#include "SyncMaster.h"
#include "config/MemorySections.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr int kDefaultTempo = 120;
constexpr int kDefaultSampleRate = 44100;

struct SlotIDs {
  FourCC target_;
  FourCC shape_;
  FourCC trigger_;
  FourCC rateMode_;
  FourCC depth_;
  FourCC rate_;
  FourCC syncIndex_;
};

constexpr SlotIDs kSlotIDs[LFOSettings::MaxLFOCount] = {
    {FourCC::LFO1Target, FourCC::LFO1Shape, FourCC::LFO1Trigger,
     FourCC::LFO1RateMode, FourCC::LFO1Depth, FourCC::LFO1HertzTenths,
     FourCC::LFO1Sync},
    {FourCC::LFO2Target, FourCC::LFO2Shape, FourCC::LFO2Trigger,
     FourCC::LFO2RateMode, FourCC::LFO2Depth, FourCC::LFO2HertzTenths,
     FourCC::LFO2Sync},
};

float sampleRate() {
  Audio *audio = Audio::GetInstance();
  const int rate = audio ? audio->GetSampleRate() : kDefaultSampleRate;
  return static_cast<float>(rate > 0 ? rate : kDefaultSampleRate);
}

int tempo() {
  SyncMaster *sync = SyncMaster::GetInstance();
  const int currentTempo = sync ? sync->GetTempo() : kDefaultTempo;
  return currentTempo > 0 ? currentTempo : kDefaultTempo;
}

float evaluateShape(LFOShape shape, float phase, float randomValue) {
  switch (shape) {
  case LFOShape::Sine:
    return std::sin(2.0f * kPi * phase);
  case LFOShape::Triangle:
    return 1.0f - 4.0f * std::abs(phase - 0.5f);
  case LFOShape::Square:
    return phase < 0.5f ? 1.0f : -1.0f;
  case LFOShape::Ramp:
    return (2.0f * phase) - 1.0f;
  case LFOShape::Random:
    return randomValue;
  case LFOShape::Last:
    break;
  }
  return 0.0f;
}

} // namespace

ChannelLFOBank::ChannelLFOBank(int channel)
    : channel_(channel), instrument_(nullptr), lastStartedInstrument_(nullptr) {
  Reset();
}

void ChannelLFOBank::SetInstrument(I_Instrument *instrument) {
  instrument_ = instrument;
  refreshParameterRefs();
}

void ChannelLFOBank::ClearInstrument() {
  instrument_ = nullptr;
  clearParameterRefs();
}

void ChannelLFOBank::OnNoteStart(bool trigger) {
  if (!instrument_) {
    return;
  }

  const bool instrumentChanged = lastStartedInstrument_ != instrument_;
  snapshotConfigs();
  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    const LFOConfig &config = configs_[i];
    if (config.target_ == LFOTarget::None) {
      states_[i].currentValue_ = 0.0f;
      continue;
    }

    if (instrumentChanged ||
        (trigger && config.triggerMode_ == LFOTriggerMode::Trigger)) {
      resetState(i);
    } else {
      states_[i].currentValue_ = evaluateCurrentValue(i);
    }
  }

  lastStartedInstrument_ = instrument_;
}

void ChannelLFOBank::Advance(int samplecount) {
  AdvanceSlots(static_cast<uint8_t>((1u << LFOSettings::MaxLFOCount) - 1u),
               samplecount);
}

PICOTRACKER_FAST_CODE("channel_lfo_advance_slots")
void ChannelLFOBank::AdvanceSlots(uint8_t slotMask, int samplecount) {
  if (!instrument_) {
    return;
  }

  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    if ((slotMask & static_cast<uint8_t>(1u << i)) == 0) {
      continue;
    }
    advanceState(i, samplecount);
  }
}

void ChannelLFOBank::Reset() {
  instrument_ = nullptr;
  lastStartedInstrument_ = nullptr;
  clearParameterRefs();
  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    states_[i].phase_ = 0.0f;
    states_[i].currentValue_ = 0.0f;
    states_[i].randomValue_ = 0.0f;
    states_[i].rngState_ =
        0x1234567u + static_cast<uint32_t>(channel_ * 17 + i);
  }
}

PICOTRACKER_FAST_CODE("channel_lfo_get_live_value")
int ChannelLFOBank::GetLiveModulatedValue(LFOTarget target, int baseValue,
                                          int minValue, int maxValue) const {
  const int configIndex = findConfigIndex(target);
  if (configIndex < 0) {
    return std::clamp(baseValue, minValue, maxValue);
  }

  const LFOConfig &config = configs_[static_cast<size_t>(configIndex)];
  return applyDepth(baseValue, minValue, maxValue,
                    states_[static_cast<size_t>(configIndex)].currentValue_,
                    config.depth_);
}

uint8_t ChannelLFOBank::GetSlotMaskForTarget(LFOTarget target) const {
  if (!instrument_ || target == LFOTarget::None) {
    return 0;
  }

  uint8_t mask = 0;
  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    const LFOConfig &config = configs_[i];
    if (config.target_ == target) {
      mask |= static_cast<uint8_t>(1u << i);
    }
  }
  return mask;
}

void ChannelLFOBank::snapshotConfigs() {
  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    configs_[i] = readConfig(i);
  }
}

void ChannelLFOBank::resetState(size_t index) {
  RuntimeState &state = states_[index];
  state.phase_ = 0.0f;
  state.randomValue_ = randomBipolar(index);
  state.currentValue_ = evaluateCurrentValue(index);
}

PICOTRACKER_FAST_CODE("channel_lfo_advance_state")
void ChannelLFOBank::advanceState(size_t index, int samplecount) {
  if (!instrument_) {
    return;
  }

  const LFOConfig &config = configs_[index];
  if (config.target_ == LFOTarget::None) {
    states_[index].currentValue_ = 0.0f;
    return;
  }

  const float increment =
      phaseIncrement(config) * static_cast<float>(samplecount);
  RuntimeState &state = states_[index];
  state.phase_ += increment;
  while (state.phase_ >= 1.0f) {
    state.phase_ -= 1.0f;
    if (config.shape_ == LFOShape::Random) {
      state.randomValue_ = randomBipolar(index);
    }
  }
  state.currentValue_ = evaluateCurrentValue(index);
}

void ChannelLFOBank::refreshParameterRefs() {
  clearParameterRefs();
  if (!instrument_) {
    return;
  }

  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    const SlotIDs &ids = kSlotIDs[i];
    SlotParameterRefs &params = params_[i];
    params.target_ = instrument_->FindVariable(ids.target_);
    params.shape_ = instrument_->FindVariable(ids.shape_);
    params.trigger_ = instrument_->FindVariable(ids.trigger_);
    params.rateMode_ = instrument_->FindVariable(ids.rateMode_);
    params.depth_ = instrument_->FindVariable(ids.depth_);
    params.rate_ = instrument_->FindVariable(ids.rate_);
    params.syncIndex_ = instrument_->FindVariable(ids.syncIndex_);
  }
}

void ChannelLFOBank::clearParameterRefs() {
  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    params_[i] = {};
    configs_[i] = {};
  }
}

LFOConfig ChannelLFOBank::readConfig(size_t index) const {
  LFOConfig config;
  if (index >= LFOSettings::MaxLFOCount) {
    return config;
  }

  const SlotParameterRefs &params = params_[index];
  if (params.target_) {
    config.target_ = static_cast<LFOTarget>(params.target_->GetInt());
  }
  if (params.shape_) {
    config.shape_ = static_cast<LFOShape>(params.shape_->GetInt());
  }
  if (params.trigger_) {
    config.triggerMode_ =
        static_cast<LFOTriggerMode>(params.trigger_->GetInt());
  }
  if (params.rateMode_) {
    config.rateMode_ = static_cast<LFORateMode>(params.rateMode_->GetInt());
  }
  if (params.depth_) {
    config.depth_ = params.depth_->GetInt();
  }
  if (params.rate_) {
    config.rate_ = params.rate_->GetInt();
  }
  if (params.syncIndex_) {
    config.syncIndex_ = params.syncIndex_->GetInt();
  }
  return config;
}

PICOTRACKER_FAST_CODE("channel_lfo_evaluate_value")
float ChannelLFOBank::evaluateCurrentValue(size_t index) const {
  if (!instrument_) {
    return 0.0f;
  }
  const LFOConfig &config = configs_[index];
  return evaluateShape(config.shape_, states_[index].phase_,
                       states_[index].randomValue_);
}

PICOTRACKER_FAST_CODE("channel_lfo_random_bipolar")
float ChannelLFOBank::randomBipolar(size_t index) {
  RuntimeState &state = states_[index];
  state.rngState_ = state.rngState_ * 1664525u + 1013904223u;
  const float normalized =
      static_cast<float>((state.rngState_ >> 8) & 0xFFFFu) / 65535.0f;
  return (normalized * 2.0f) - 1.0f;
}

PICOTRACKER_FAST_CODE("channel_lfo_phase_increment")
float ChannelLFOBank::phaseIncrement(const LFOConfig &config) const {
  if (config.rateMode_ == LFORateMode::Sync) {
    const LFOSyncDivision &division =
        GetLFOSyncDivision(static_cast<size_t>(config.syncIndex_));
    return (division.cyclesPerBeat_ * static_cast<float>(tempo()) / 60.0f) /
           sampleRate();
  }

  return LFOSettings::RateValueToHertz(config.rate_) / sampleRate();
}

PICOTRACKER_FAST_CODE("channel_lfo_find_config")
int ChannelLFOBank::findConfigIndex(LFOTarget target) const {
  if (!instrument_ || target == LFOTarget::None) {
    return -1;
  }

  for (size_t i = 0; i < LFOSettings::MaxLFOCount; ++i) {
    const LFOConfig &config = configs_[i];
    if (config.target_ == target) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

PICOTRACKER_FAST_CODE("channel_lfo_apply_depth")
int ChannelLFOBank::applyDepth(int baseValue, int minValue, int maxValue,
                               float normalized, int depth) {
  const int offset = static_cast<int>(std::lround(normalized * depth));
  return std::clamp(baseValue + offset, minValue, maxValue);
}
