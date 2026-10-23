/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_SLOT_H_
#define _OUTPUT_EFFECT_SLOT_H_

#include "OutputEffectProcessor.h"
#include "Services/Audio/AudioDriver.h"
#include "Services/Audio/AudioModule.h"

class OutputEffectSlot : public AudioModule {
public:
  OutputEffectSlot(AudioSample *wetBuffer,
                   OutputEffectProcessor *defaultEffect);
  virtual ~OutputEffectSlot(){};

  virtual bool Render(AudioSample *buffer, int samplecount) override;

  void BeginRenderCycle(AudioSample returnGain);
  void ClearAccumulationBuffer();
  void ResetState();
  AudioSample *GetAccumulationBuffer();
  void SetEffect(OutputEffectProcessor *effect);
  OutputEffectProcessor *GetEffect() const;

private:
  AudioSample returnGain_;
  AudioSample *const wetBuffer_;
  OutputEffectProcessor *const defaultEffect_;
  OutputEffectProcessor *effect_;
};

#endif
