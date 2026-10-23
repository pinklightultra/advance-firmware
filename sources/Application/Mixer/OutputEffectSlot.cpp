/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "OutputEffectSlot.h"
#include "System/Console/n_assert.h"
#include "config/MemorySections.h"
#include <string.h>

OutputEffectSlot::OutputEffectSlot(AudioSample *wetBuffer,
                                   OutputEffectProcessor *defaultEffect)
    : returnGain_(0), wetBuffer_(wetBuffer), defaultEffect_(defaultEffect),
      effect_(defaultEffect) {
  NAssert(wetBuffer_);
  NAssert(defaultEffect_);
  memset(wetBuffer_, 0, MAX_SAMPLE_COUNT * 2 * sizeof(AudioSample));
}

PICOTRACKER_FAST_CODE("output_effect_slot_begin_render_cycle")
void OutputEffectSlot::BeginRenderCycle(AudioSample returnGain) {
  returnGain_ = returnGain;
}

PICOTRACKER_FAST_CODE("output_effect_slot_clear_accumulation")
void OutputEffectSlot::ClearAccumulationBuffer() {
  memset(wetBuffer_, 0, MAX_SAMPLE_COUNT * 2 * sizeof(AudioSample));
}

void OutputEffectSlot::ResetState() {
  ClearAccumulationBuffer();
  effect_->Reset();
}

bool OutputEffectSlot::IsTailActive() const { return effect_->IsTailActive(); }

AudioSample *OutputEffectSlot::GetAccumulationBuffer() { return wetBuffer_; }

void OutputEffectSlot::SetEffect(OutputEffectProcessor *effect) {
  effect_ = effect ? effect : defaultEffect_;
}

OutputEffectProcessor *OutputEffectSlot::GetEffect() const { return effect_; }

PICOTRACKER_FAST_CODE("output_effect_slot_render")
bool OutputEffectSlot::Render(AudioSample *buffer, int samplecount) {
  const int count = samplecount * 2;
  effect_->Render(wetBuffer_, buffer, samplecount);

  for (int i = 0; i < count; ++i) {
    buffer[i] *= returnGain_;
  }

  return true;
}
