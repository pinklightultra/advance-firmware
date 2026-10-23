/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "InsertEffectSlot.h"
#include "System/Console/n_assert.h"

InsertEffectSlot::InsertEffectSlot() = default;

void InsertEffectSlot::ResetState() {
  NAssert(effect_);
  effect_->Reset();
}

void InsertEffectSlot::SetEffect(InsertEffectProcessor *effect) {
  NAssert(effect);
  effect_ = effect;
}

InsertEffectProcessor *InsertEffectSlot::GetEffect() const { return effect_; }

void InsertEffectSlot::Render(AudioSample *buffer, int samplecount, int amount,
                              AudioBufferLayout layout) {
  NAssert(effect_);
  effect_->Render(buffer, samplecount, amount, layout);
}
