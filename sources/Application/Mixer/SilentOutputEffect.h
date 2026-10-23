/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SILENT_OUTPUT_EFFECT_H_
#define _SILENT_OUTPUT_EFFECT_H_

#include "OutputEffectProcessor.h"

class SilentOutputEffect : public OutputEffectProcessor {
public:
  virtual ~SilentOutputEffect(){};

  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    (void)input;

    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = kAudioSampleZero;
    }
  }

  virtual const char *GetShortName() const override { return "NONE"; }
};

#endif
