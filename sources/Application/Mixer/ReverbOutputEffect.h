/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _REVERB_OUTPUT_EFFECT_H_
#define _REVERB_OUTPUT_EFFECT_H_

#include "OutputEffectProcessor.h"

#include <cstdint>

#define REVERB_OUTPUT_EFFECT_ALGORITHM_FREEVERB 1
#define REVERB_OUTPUT_EFFECT_ALGORITHM_RINGS 2
#define REVERB_OUTPUT_EFFECT_ALGORITHM_SCHROEDER                               \
  REVERB_OUTPUT_EFFECT_ALGORITHM_FREEVERB

#ifndef REVERB_OUTPUT_EFFECT_ALGORITHM
#define REVERB_OUTPUT_EFFECT_ALGORITHM REVERB_OUTPUT_EFFECT_ALGORITHM_RINGS
#endif

#if REVERB_OUTPUT_EFFECT_ALGORITHM == REVERB_OUTPUT_EFFECT_ALGORITHM_RINGS
#include "rings-reverb-impl.h"
using ReverbOutputEffectImplementation = RingsReverb;
#elif REVERB_OUTPUT_EFFECT_ALGORITHM == REVERB_OUTPUT_EFFECT_ALGORITHM_FREEVERB
#include "freeverb-reverb.h"
using ReverbOutputEffectImplementation = FreeverbReverb;
#else
#error "Unsupported REVERB_OUTPUT_EFFECT_ALGORITHM"
#endif

class ReverbOutputEffect : public OutputEffectProcessor {
public:
  ReverbOutputEffect();
  virtual ~ReverbOutputEffect(){};

  void SetParameters(float inputGain, float time, float diffusion,
                     float lowPass);
  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override;
  virtual const char *GetShortName() const override { return "REVB"; }

private:
  bool HasAudibleInput(const AudioSample *input, int32_t sampleCount) const;
  void Reset();

  ReverbOutputEffectImplementation implementation_;
  float inputGain_;
  float time_;
  float diffusion_;
  float lowPass_;
  int32_t tailFramesRemaining_ = 0;
};

#endif
