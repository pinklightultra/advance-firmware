/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "OutputEffectSlot.h"
#include "System/Console/n_assert.h"
#include "System/Profiler/DWTProfiler.h"
#include "config/MemorySections.h"
#include <string.h>

DWTProfileStats *OutputEffectSlot::mixProfileStats_ = nullptr;

OutputEffectSlot::OutputEffectSlot(AudioSample *wetBuffer, OutputEffect &effect)
    : returnGain_(0), tailFramesRemaining_(0), wetBuffer_(wetBuffer),
      effect_(&effect) {
  NAssert(wetBuffer_);
  memset(wetBuffer_, 0, MAX_SAMPLE_COUNT * 2 * sizeof(AudioSample));
  ClearSendRouting();
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
  tailFramesRemaining_ = 0;
  effect_->Reset();
}

bool OutputEffectSlot::IsTailActive() const { return tailFramesRemaining_ > 0; }

AudioSample *OutputEffectSlot::GetAccumulationBuffer() { return wetBuffer_; }

void OutputEffectSlot::ClearSendRouting() { sendRoutingCount_ = 0; }

bool OutputEffectSlot::AddSendRouting(AudioSample *buffer,
                                      AudioSample sendGain) {
  if (buffer == nullptr || sendRoutingCount_ >= OUTPUT_EFFECT_COUNT) {
    return false;
  }

  sendBuffers_[sendRoutingCount_] = buffer;
  sendGains_[sendRoutingCount_] = sendGain;
  ++sendRoutingCount_;
  return true;
}

void OutputEffectSlot::SetMixProfileStats(DWTProfileStats *stats) {
  mixProfileStats_ = stats;
}

OutputEffect &OutputEffectSlot::GetEffect() { return *effect_; }

const OutputEffect &OutputEffectSlot::GetEffect() const { return *effect_; }

PICOTRACKER_FAST_CODE("output_effect_slot_has_input")
bool OutputEffectSlot::HasInput(int count) const {
  for (int i = 0; i < count; ++i) {
    if (wetBuffer_[i] != kAudioSampleZero) {
      // Once input is present, the processor must run and no further scan can
      // change that decision.
      return true;
    }
  }
  return false;
}

PICOTRACKER_FAST_CODE("output_effect_slot_render")
bool OutputEffectSlot::Render(AudioSample *buffer, int samplecount) {
  const int count = samplecount * 2;
  const bool hasInput = HasInput(count);

  if (hasInput || tailFramesRemaining_ > 0) {
    effect_->PrepareRender();
  }

  if (hasInput) {
    const int32_t tailFrameCount = effect_->GetTailFrameCount();
    tailFramesRemaining_ = tailFrameCount > 0 ? tailFrameCount : 0;
  }

  if (hasInput || tailFramesRemaining_ > 0) {
    effect_->Render(wetBuffer_, buffer, samplecount);
  } else {
    memset(buffer, 0, static_cast<size_t>(count) * sizeof(AudioSample));
  }

  if (!hasInput && tailFramesRemaining_ > 0) {
    tailFramesRemaining_ -= samplecount;
    if (tailFramesRemaining_ < 0) {
      tailFramesRemaining_ = 0;
    }
  }

  // The owning output-effect bus attributes the complete fixed effect to its
  // individual profile. Also attribute only the shared post-processor routing
  // and return math to the mixer total.
  AccumulatingDWTProfile mixProfile(mixProfileStats_);
  mixProfile.Begin();

  // Downstream sends are pre-return so one effect can feed a later effect while
  // remaining absent from the master mix. Every legal route is processed even
  // at zero gain so routing always has its worst-case execution cost.
  for (uint8_t send = 0; send < sendRoutingCount_; ++send) {
    AudioSample *sendBuffer = sendBuffers_[send];
    const AudioSample sendGain = sendGains_[send];
    for (int i = 0; i < count; ++i) {
      sendBuffer[i] += buffer[i] * sendGain;
    }
  }

  for (int i = 0; i < count; ++i) {
    buffer[i] *= returnGain_;
  }
  mixProfile.End();

  return true;
}
