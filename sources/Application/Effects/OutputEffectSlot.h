/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_SLOT_H_
#define _OUTPUT_EFFECT_SLOT_H_

#include "OutputEffect.h"
#include "OutputEffectConstants.h"
#include "Services/Audio/AudioDriver.h"
#include "Services/Audio/AudioModule.h"
#include <stdint.h>

struct DWTProfileStats;

class OutputEffectSlot : public AudioModule {
public:
  OutputEffectSlot(AudioSample *wetBuffer, OutputEffect &effect);
  virtual ~OutputEffectSlot(){};

  virtual bool Render(AudioSample *buffer, int samplecount) override;

  void BeginRenderCycle(AudioSample returnGain);
  void ClearAccumulationBuffer();
  void ResetState();
  bool IsTailActive() const;
  AudioSample *GetAccumulationBuffer();
  void ClearSendRouting();
  bool AddSendRouting(AudioSample *buffer, AudioSample sendGain);
  static void SetMixProfileStats(DWTProfileStats *stats);
  OutputEffect &GetEffect();
  const OutputEffect &GetEffect() const;

private:
  bool HasInput(int count) const;

  AudioSample returnGain_;
  int32_t tailFramesRemaining_;
  AudioSample *const wetBuffer_;
  OutputEffect *effect_;
  AudioSample *sendBuffers_[OUTPUT_EFFECT_COUNT];
  AudioSample sendGains_[OUTPUT_EFFECT_COUNT];
  uint8_t sendRoutingCount_;
  static DWTProfileStats *mixProfileStats_;
};

#endif
