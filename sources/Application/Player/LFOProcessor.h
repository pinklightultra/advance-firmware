/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _LFO_PROCESSOR_H_
#define _LFO_PROCESSOR_H_

#include "Application/Instruments/I_Instrument.h"
#include "Application/Instruments/LFOSettings.h"
#include <cstdint>

class ChannelLFOBank {
public:
  explicit ChannelLFOBank(int channel);

  void SetInstrument(I_Instrument *instrument);
  void ClearInstrument();
  void OnNoteStart(bool trigger);
  void Advance(int samplecount);
  void AdvanceSlots(uint8_t slotMask, int samplecount);
  void Reset();

  int GetLiveModulatedValue(LFOTarget target, int baseValue, int minValue,
                            int maxValue) const;
  uint8_t GetSlotMaskForTarget(LFOTarget target) const;

private:
  struct SlotParameterRefs {
    Variable *target_ = nullptr;
    Variable *shape_ = nullptr;
    Variable *trigger_ = nullptr;
    Variable *rateMode_ = nullptr;
    Variable *depth_ = nullptr;
    Variable *rate_ = nullptr;
    Variable *syncIndex_ = nullptr;
  };

  struct RuntimeState {
    float phase_ = 0.0f;
    float currentValue_ = 0.0f;
    float randomValue_ = 0.0f;
    uint32_t rngState_ = 1;
  };

  void snapshotConfigs();
  void resetState(size_t index);
  void advanceState(size_t index, int samplecount);
  void refreshParameterRefs();
  void clearParameterRefs();
  LFOConfig readConfig(size_t index) const;
  float evaluateCurrentValue(size_t index) const;
  float randomBipolar(size_t index);
  float phaseIncrement(const LFOConfig &config) const;
  int findConfigIndex(LFOTarget target) const;
  static int applyDepth(int baseValue, int minValue, int maxValue,
                        float normalized, int depth);

  int channel_;
  I_Instrument *instrument_;
  I_Instrument *lastStartedInstrument_;
  SlotParameterRefs params_[LFOSettings::MaxLFOCount];
  LFOConfig configs_[LFOSettings::MaxLFOCount];
  RuntimeState states_[LFOSettings::MaxLFOCount];
};

#endif
