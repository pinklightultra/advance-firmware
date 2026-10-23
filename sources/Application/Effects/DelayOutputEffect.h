/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DELAY_OUTPUT_EFFECT_H_
#define _DELAY_OUTPUT_EFFECT_H_

#include "Application/Model/TempoConstants.h"
#include "OutputEffect.h"
#include "Services/Audio/AudioConstants.h"

// A deliberately simple delay that keeps its complete stereo history in
// external memory. Every frame performs one stereo write and one stereo read.
class DelayOutputEffect final : public OutputEffect {
public:
  static constexpr size_t kInternalMemorySize = 0;

  // TIME is stored in sequencer ticks so its values align with audio rendering
  // boundaries. A 16-step bar contains 96 ticks.
  static constexpr int TicksPerStep = 6;
  static constexpr int TicksPerBar = TicksPerStep * 16;
  static constexpr int MinimumTime = 1;
  static constexpr int MaximumTime = TicksPerBar;
  static constexpr int DefaultTime = MaximumTime;
  static constexpr uint8_t DefaultFeedback = 0;
  static constexpr uint8_t MaximumFeedback = 0xFF;
  // Keep the maximum feedback just below unity so every echo decays.
  static constexpr uint16_t FeedbackScale = 0x100;

  // Allocate enough stereo frames for the longest selectable delay: one
  // four-beat bar at the fixed engine rate and minimum supported tempo.
  static constexpr uint32_t SecondsPerMinute = 60;
  static constexpr uint32_t BeatsPerBar = 4;
  static constexpr uint32_t kBufferFrameCount =
      kAudioSampleRateHz * SecondsPerMinute * BeatsPerBar / MIN_TEMPO;
  static constexpr size_t kExternalMemorySize =
      static_cast<size_t>(kBufferFrameCount) * sizeof(AudioSample) * 2U;

  DelayOutputEffect();

  int32_t GetTailFrameCount() const override;
  void Reset() override;
  void Render(const AudioSample *input, AudioSample *output,
              int samplecount) override;
  int GetTime() const { return time_.GetInt(); }
  int GetFeedback() const { return feedback_.GetInt(); }

private:
  struct StereoFrame {
    AudioSample left_;
    AudioSample right_;
  };

  etl::ivector<Variable *> *Variables() override { return &variables_; }
  void ApplyParameters() override;
  void SetParameters(uint8_t time, uint8_t feedback);
  void RefreshDelayFrames();
  void UpdateFeedbackTailRepeatCount();

  static constexpr size_t VariableCount = 2;

  etl::vector<Variable *, VariableCount> variables_;
  WatchedVariable time_;
  WatchedVariable feedback_;
  StereoFrame *delayBuffer_ = nullptr;
  uint32_t writeFrame_ = 0;
  uint32_t delayFrames_ = 1;
  uint8_t timeValue_ = DefaultTime;
  uint16_t feedbackTailRepeatCount_ = 1;
  AudioSample feedbackGain_ = kAudioSampleZero;
};

static_assert(DelayOutputEffect::kExternalMemorySize %
                      (sizeof(AudioSample) * 2U) ==
                  0,
              "Delay memory must contain complete stereo frames");
static_assert(DelayOutputEffect::MaximumTime == 96,
              "A full 16-step bar must contain 96 delay ticks");

#endif
