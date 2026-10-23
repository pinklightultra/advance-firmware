/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "OutputEffectSlot.h"
#include "System/Console/n_assert.h"
#include <string.h>

OutputEffectSlot::OutputEffectSlot(AudioSample *wetBuffer,
                                   OutputEffectProcessor *defaultEffect)
    : returnGain_(0), wetBuffer_(wetBuffer), defaultEffect_(defaultEffect),
      effect_(defaultEffect) {
  NAssert(wetBuffer_);
  NAssert(defaultEffect_);
  memset(wetBuffer_, 0, MAX_SAMPLE_COUNT * 2 * sizeof(AudioSample));
}

void OutputEffectSlot::BeginRenderCycle(AudioSample returnGain) {
  returnGain_ = returnGain;
}

void OutputEffectSlot::ClearAccumulationBuffer() {
  memset(wetBuffer_, 0, MAX_SAMPLE_COUNT * 2 * sizeof(AudioSample));
}

void OutputEffectSlot::ResetState() {
  ClearAccumulationBuffer();
  effect_->Reset();
}

AudioSample *OutputEffectSlot::GetAccumulationBuffer() { return wetBuffer_; }

void OutputEffectSlot::SetEffect(OutputEffectProcessor *effect) {
  effect_ = effect ? effect : defaultEffect_;
}

OutputEffectProcessor *OutputEffectSlot::GetEffect() const { return effect_; }

bool OutputEffectSlot::Render(AudioSample *buffer, int samplecount) {
  const int count = samplecount * 2;
  effect_->Render(wetBuffer_, buffer, samplecount);

  for (int i = 0; i < count; ++i) {
    buffer[i] *= returnGain_;
  }

  return true;
}
