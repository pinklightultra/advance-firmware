/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "LFOSettings.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr LFOTargetInfo kTargetInfos[static_cast<size_t>(LFOTarget::Last)] = {
    {LFOTarget::None, "OFF"},
    {LFOTarget::VoiceBandpassCutoff, "BASE"},
    {LFOTarget::VoiceBandpassMix, "WDTH"},
    {LFOTarget::VoicePan, "PAN"},
    {LFOTarget::VoiceInsertAmount1, "FX1"},
    {LFOTarget::VoiceInsertAmount2, "FX2"},
    {LFOTarget::VoiceFilterCutoff, "FCOF"},
    {LFOTarget::VoiceFilterResonance, "FRES"},
    {LFOTarget::VoiceFilterEnvAmount, "FENV"},
    {LFOTarget::VoiceFilterAttack, "FATK"},
    {LFOTarget::VoiceFilterDecay, "FDEC"},
    {LFOTarget::VoiceFilterSustain, "FSUS"},
    {LFOTarget::VoiceFilterRelease, "FREL"},
    {LFOTarget::VoiceAmp, "AMP"},
    {LFOTarget::VoiceAmpAttack, "AATK"},
    {LFOTarget::VoiceAmpDecay, "ADEC"},
    {LFOTarget::VoiceAmpSustain, "ASUS"},
    {LFOTarget::VoiceAmpRelease, "AREL"},
};

constexpr const char *kShapeNames[static_cast<size_t>(LFOShape::Last)] = {
    "sine", "triangle", "square", "ramp", "random"};
constexpr const char *kTriggerModeNames[static_cast<size_t>(
    LFOTriggerMode::Last)] = {"free", "trig"};
constexpr const char *kRateModeNames[static_cast<size_t>(LFORateMode::Last)] = {
    "spd", "syn"};

constexpr LFOSyncDivision kSyncDivisions[] = {
    {"4/1", 0.25f}, {"2/1", 0.5f},  {"1/1", 1.0f},   {"1/2", 2.0f},
    {"1/4", 4.0f},  {"1/8", 8.0f},  {"1/16", 16.0f}, {"1/32", 32.0f},
    {"1/2T", 3.0f}, {"1/4T", 6.0f}, {"1/8T", 12.0f}, {"1/16T", 24.0f},
};

bool isVoiceTarget(LFOTarget target) {
  return target > LFOTarget::None && target < LFOTarget::Last;
}

LFOTarget normalizeTarget(LFOTarget target) {
  return isVoiceTarget(target) ? target : LFOTarget::None;
}

LFOShape normalizeShape(LFOShape shape) {
  return static_cast<size_t>(shape) < static_cast<size_t>(LFOShape::Last)
             ? shape
             : LFOShape::Sine;
}

LFOTriggerMode normalizeTriggerMode(LFOTriggerMode mode) {
  return static_cast<size_t>(mode) < static_cast<size_t>(LFOTriggerMode::Last)
             ? mode
             : LFOTriggerMode::Trigger;
}

LFORateMode normalizeRateMode(LFORateMode mode) {
  return static_cast<size_t>(mode) < static_cast<size_t>(LFORateMode::Last)
             ? mode
             : LFORateMode::Hertz;
}

int getVoiceTargetMin(LFOTarget target) {
  return target == LFOTarget::VoiceFilterEnvAmount ? -64 : 0;
}

int getVoiceTargetMax(LFOTarget target) {
  (void)target;
  if (target == LFOTarget::VoiceFilterEnvAmount) {
    return 64;
  }

  return 0xFF;
}

} // namespace

LFOSettings::SlotVariables::SlotVariables(FourCC targetId, FourCC shapeId,
                                          FourCC triggerId, FourCC rateModeId,
                                          FourCC depthId, FourCC rateId,
                                          FourCC syncId)
    : target_(targetId, static_cast<int>(LFOTarget::None)),
      shape_(shapeId, static_cast<int>(LFOShape::Sine)),
      trigger_(triggerId, static_cast<int>(LFOTriggerMode::Trigger)),
      rateMode_(rateModeId, static_cast<int>(LFORateMode::Hertz)),
      depth_(depthId, 0), rate_(rateId, DefaultRateValue),
      syncIndex_(syncId, 4) {}

void LFOSettings::SlotVariables::AppendTo(etl::ivector<Variable *> &variables) {
  variables.push_back(&target_);
  variables.push_back(&shape_);
  variables.push_back(&trigger_);
  variables.push_back(&rateMode_);
  variables.push_back(&depth_);
  variables.push_back(&rate_);
  variables.push_back(&syncIndex_);
}

void LFOSettings::SlotVariables::Reset() {
  target_.Reset();
  shape_.Reset();
  trigger_.Reset();
  rateMode_.Reset();
  depth_.Reset();
  rate_.Reset();
  syncIndex_.Reset();
}

LFOSettings::LFOSettings()
    : VariableContainer(&variables_),
      slot0_(FourCC::LFO1Target, FourCC::LFO1Shape, FourCC::LFO1Trigger,
             FourCC::LFO1RateMode, FourCC::LFO1Depth, FourCC::LFO1HertzTenths,
             FourCC::LFO1Sync),
      slot1_(FourCC::LFO2Target, FourCC::LFO2Shape, FourCC::LFO2Trigger,
             FourCC::LFO2RateMode, FourCC::LFO2Depth, FourCC::LFO2HertzTenths,
             FourCC::LFO2Sync) {
  slot0_.AppendTo(variables_);
  slot1_.AppendTo(variables_);
}

void LFOSettings::AppendTo(etl::ivector<Variable *> &variables) {
  for (auto it = variables_.begin(); it != variables_.end(); ++it) {
    variables.push_back(*it);
  }
}

void LFOSettings::Reset() {
  slot0_.Reset();
  slot1_.Reset();
}

void LFOSettings::CopyFrom(const LFOSettings &other) {
  auto src = other.variables_.begin();
  for (auto dst = variables_.begin();
       dst != variables_.end() && src != other.variables_.end(); ++dst, ++src) {
    (*dst)->CopyFrom(const_cast<Variable &>(**src));
  }
}

bool LFOSettings::IsTargetInUse(LFOTarget target, size_t exceptIndex) const {
  if (target == LFOTarget::None) {
    return false;
  }

  for (size_t i = 0; i < MaxLFOCount; ++i) {
    if (i == exceptIndex) {
      continue;
    }
    if (Get(i).target_ == target) {
      return true;
    }
  }
  return false;
}

void LFOSettings::SetTarget(size_t index, LFOTarget target) {
  if (index >= MaxLFOCount) {
    return;
  }

  target = normalizeTarget(target);

  for (size_t i = 0; i < MaxLFOCount; ++i) {
    if (i != index && Get(i).target_ == target && target != LFOTarget::None) {
      writeInt(slot(i).target_, static_cast<int>(LFOTarget::None));
    }
  }

  writeInt(slot(index).target_, static_cast<int>(target));
}

void LFOSettings::SetShape(size_t index, LFOShape shape) {
  if (index >= MaxLFOCount) {
    return;
  }
  writeInt(slot(index).shape_, static_cast<int>(normalizeShape(shape)));
}

void LFOSettings::SetTriggerMode(size_t index, LFOTriggerMode mode) {
  if (index >= MaxLFOCount) {
    return;
  }
  writeInt(slot(index).trigger_, static_cast<int>(normalizeTriggerMode(mode)));
}

void LFOSettings::SetRateMode(size_t index, LFORateMode mode) {
  if (index >= MaxLFOCount) {
    return;
  }
  writeInt(slot(index).rateMode_, static_cast<int>(normalizeRateMode(mode)));
}

void LFOSettings::SetDepth(size_t index, int depth) {
  if (index >= MaxLFOCount) {
    return;
  }
  writeInt(slot(index).depth_, std::max(depth, 0));
}

void LFOSettings::SetRateValue(size_t index, int rateValue) {
  if (index >= MaxLFOCount) {
    return;
  }
  writeInt(slot(index).rate_,
           std::clamp(rateValue, MinRateValue, MaxRateValue));
}

void LFOSettings::SetSyncIndex(size_t index, int syncIndex) {
  if (index >= MaxLFOCount) {
    return;
  }
  writeInt(slot(index).syncIndex_,
           std::clamp(syncIndex, 0,
                      static_cast<int>(GetLFOSyncDivisionCount()) - 1));
}

LFOConfig LFOSettings::Get(size_t index) const {
  LFOConfig config;
  if (index >= MaxLFOCount) {
    return config;
  }

  const SlotVariables &variables = slot(index);
  config.target_ = static_cast<LFOTarget>(readInt(variables.target_));
  config.shape_ = static_cast<LFOShape>(readInt(variables.shape_));
  config.triggerMode_ =
      static_cast<LFOTriggerMode>(readInt(variables.trigger_));
  config.rateMode_ = static_cast<LFORateMode>(readInt(variables.rateMode_));
  config.depth_ = readInt(variables.depth_);
  config.rate_ = readInt(variables.rate_);
  config.syncIndex_ = readInt(variables.syncIndex_);
  return config;
}

float LFOSettings::RateValueToHertz(int rateValue) {
  const float normalized =
      static_cast<float>(std::clamp(rateValue, MinRateValue, MaxRateValue)) /
      static_cast<float>(MaxRateValue);
  return MinRateHz * std::pow(MaxRateHz / MinRateHz, normalized);
}

LFOSettings::SlotVariables &LFOSettings::slot(size_t index) {
  return index == 0 ? slot0_ : slot1_;
}

const LFOSettings::SlotVariables &LFOSettings::slot(size_t index) const {
  return index == 0 ? slot0_ : slot1_;
}

int LFOSettings::readInt(const Variable &variable) {
  return const_cast<Variable &>(variable).GetInt();
}

void LFOSettings::writeInt(Variable &variable, int value) {
  variable.SetInt(value, false);
}

const LFOTargetInfo &GetLFOTargetInfo(LFOTarget target) {
  const size_t index = static_cast<size_t>(target);
  if (index >= static_cast<size_t>(LFOTarget::Last)) {
    return kTargetInfos[0];
  }
  return kTargetInfos[index];
}

const char *GetLFOShapeName(LFOShape shape) {
  const size_t index = static_cast<size_t>(shape);
  if (index >= static_cast<size_t>(LFOShape::Last)) {
    return kShapeNames[0];
  }
  return kShapeNames[index];
}

const char *GetLFOTriggerModeName(LFOTriggerMode mode) {
  const size_t index = static_cast<size_t>(mode);
  if (index >= static_cast<size_t>(LFOTriggerMode::Last)) {
    return kTriggerModeNames[0];
  }
  return kTriggerModeNames[index];
}

const char *GetLFORateModeName(LFORateMode mode) {
  const size_t index = static_cast<size_t>(mode);
  if (index >= static_cast<size_t>(LFORateMode::Last)) {
    return kRateModeNames[0];
  }
  return kRateModeNames[index];
}

const LFOSyncDivision &GetLFOSyncDivision(size_t index) {
  if (index >= GetLFOSyncDivisionCount()) {
    return kSyncDivisions[0];
  }
  return kSyncDivisions[index];
}

size_t GetLFOSyncDivisionCount() {
  return sizeof(kSyncDivisions) / sizeof(kSyncDivisions[0]);
}

bool GetLFOTargetValueRange(LFOTarget target, int &minValue, int &maxValue) {
  if (isVoiceTarget(target)) {
    minValue = getVoiceTargetMin(target);
    maxValue = getVoiceTargetMax(target);
    return true;
  }

  switch (target) {
  case LFOTarget::None:
    minValue = 0;
    maxValue = 0;
    return true;

  case LFOTarget::Last:
    break;
  default:
    break;
  }

  return false;
}
