/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _LFO_SETTINGS_H_
#define _LFO_SETTINGS_H_

#include "Externals/etl/include/etl/vector.h"
#include "Foundation/Types/Types.h"
#include "Foundation/Variables/Variable.h"
#include "Foundation/Variables/VariableContainer.h"
#include <cstddef>
#include <cstdint>

class I_Instrument;

enum class LFOTarget : uint8_t {
  None = 0,
  VoiceBandpassCutoff,
  VoiceBandpassMix,
  VoicePan,
  VoiceInsertAmount1,
  VoiceInsertAmount2,
  VoiceFilterCutoff,
  VoiceFilterResonance,
  VoiceFilterEnvAmount,
  VoiceFilterAttack,
  VoiceFilterDecay,
  VoiceFilterSustain,
  VoiceFilterRelease,
  VoiceAmp,
  VoiceAmpAttack,
  VoiceAmpDecay,
  VoiceAmpSustain,
  VoiceAmpRelease,
  Last
};

enum class LFOShape : uint8_t {
  Sine = 0,
  Triangle,
  Square,
  Ramp,
  Random,
  Last
};

enum class LFOTriggerMode : uint8_t { FreeRun = 0, Trigger, Last };

enum class LFORateMode : uint8_t { Hertz = 0, Sync, Last };

struct LFOConfig {
  LFOTarget target_ = LFOTarget::None;
  LFOShape shape_ = LFOShape::Sine;
  LFOTriggerMode triggerMode_ = LFOTriggerMode::Trigger;
  LFORateMode rateMode_ = LFORateMode::Hertz;
  int depth_ = 0;
  int rate_ = 160;
  int syncIndex_ = 4;
};

struct LFOTargetInfo {
  LFOTarget target_ = LFOTarget::None;
  const char *name_ = "off";
};

struct LFOSyncDivision {
  const char *name_ = "1/4";
  float cyclesPerBeat_ = 1.0f;
};

class LFOSettings : public VariableContainer {
public:
  static constexpr size_t MaxLFOCount = 2;
  static constexpr size_t SlotVariableCount = 7;
  static constexpr size_t VariableCount = MaxLFOCount * SlotVariableCount;
  static constexpr int MinRateValue = 0;
  static constexpr int MaxRateValue = 0xFF;
  static constexpr int DefaultRateValue = 160; // ~1 Hz with the current curve
  static constexpr float MinRateHz = 0.01f;
  static constexpr float MaxRateHz = 15.0f;

  LFOSettings();

  void AppendTo(etl::ivector<Variable *> &variables);
  void Reset();
  void CopyFrom(const LFOSettings &other);
  bool IsTargetInUse(LFOTarget target, size_t exceptIndex) const;
  void SetTarget(size_t index, LFOTarget target);
  void SetShape(size_t index, LFOShape shape);
  void SetTriggerMode(size_t index, LFOTriggerMode mode);
  void SetRateMode(size_t index, LFORateMode mode);
  void SetDepth(size_t index, int depth);
  void SetRateValue(size_t index, int rateValue);
  void SetSyncIndex(size_t index, int syncIndex);
  LFOConfig Get(size_t index) const;
  static float RateValueToHertz(int rateValue);
  etl::ivector<Variable *> *Variables() { return &variables_; }

private:
  struct SlotVariables {
    SlotVariables(FourCC targetId, FourCC shapeId, FourCC triggerId,
                  FourCC rateModeId, FourCC depthId, FourCC rateId,
                  FourCC syncId);

    void AppendTo(etl::ivector<Variable *> &variables);
    void Reset();

    Variable target_;
    Variable shape_;
    Variable trigger_;
    Variable rateMode_;
    Variable depth_;
    Variable rate_;
    Variable syncIndex_;
  };

  SlotVariables &slot(size_t index);
  const SlotVariables &slot(size_t index) const;
  static int readInt(const Variable &variable);
  static void writeInt(Variable &variable, int value);

  etl::vector<Variable *, VariableCount> variables_;
  SlotVariables slot0_;
  SlotVariables slot1_;
};

const LFOTargetInfo &GetLFOTargetInfo(LFOTarget target);
const char *GetLFOShapeName(LFOShape shape);
const char *GetLFOTriggerModeName(LFOTriggerMode mode);
const char *GetLFORateModeName(LFORateMode mode);
const LFOSyncDivision &GetLFOSyncDivision(size_t index);
size_t GetLFOSyncDivisionCount();

bool IsLFOTargetSupportedForInstrument(int instrumentType, LFOTarget target);
bool GetLFOTargetValueRange(I_Instrument *instrument, LFOTarget target,
                            int &minValue, int &maxValue);

#endif
