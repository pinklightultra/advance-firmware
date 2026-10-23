/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _FREEVERB_REVERB_H_
#define _FREEVERB_REVERB_H_

#include "Application/Audio/AudioSample.h"
#include "Application/Utils/fixed.h"
#include "System/Console/n_assert.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace freeverb_reverb_detail {

constexpr uint32_t kSampleRateHz = 48000;
constexpr int32_t kOneQ15 = 32767;
constexpr uint32_t kPreDelaySamples = 960;
constexpr uint32_t kCombStorageSizeLeft =
    509 + 863 + 1481 + 2521 + 4273 + 7253 + 10007 + 15013;
constexpr uint32_t kCombStorageSizeRight =
    523 + 877 + 1489 + 2531 + 4283 + 7283 + 10037 + 15031;
constexpr uint32_t kAllpassStorageSizeLeft = 142 + 396 + 1071 + 3079;
constexpr uint32_t kAllpassStorageSizeRight = 145 + 399 + 1073 + 3081;
constexpr int16_t kWetQ15 = static_cast<int16_t>(0.50f * 32767.0f + 0.5f);
constexpr int16_t kAllpassGainQ15 =
    static_cast<int16_t>(0.50f * 32767.0f + 0.5f);
constexpr int32_t kMasterGainQ15 =
    static_cast<int32_t>(1.50f * 32767.0f + 0.5f);
constexpr int16_t kDampQ15 = static_cast<int16_t>(0.40f * 32767.0f + 0.5f);
constexpr float kMaxFeedback = 0.98f;

inline int16_t SaturateQ15(int32_t value) {
  if (value > 0x7FFF) {
    return 0x7FFF;
  }
  if (value < -0x8000) {
    return static_cast<int16_t>(-0x8000);
  }

  return static_cast<int16_t>(value);
}

inline int16_t MultiplyQ15(int16_t left, int16_t right) {
  return SaturateQ15(
      (static_cast<int32_t>(left) * static_cast<int32_t>(right)) >> 15);
}

inline int16_t ScaleQ15(int16_t sample, int32_t gainQ15) {
  return SaturateQ15((static_cast<int32_t>(sample) * gainQ15) >> 15);
}

inline int16_t FixedToQ15(fixed value) { return SaturateQ15(fp2i(value)); }

inline fixed Q15ToFixed(int16_t value) {
  return i2fp(static_cast<int32_t>(value));
}

inline int16_t AudioSampleToQ15(AudioSample value) {
  if (value > kAudioSampleMax) {
    value = kAudioSampleMax;
  }
  if (value < kAudioSampleMin) {
    value = kAudioSampleMin;
  }

  return SaturateQ15(static_cast<int32_t>(value * kAudioPcmScale));
}

inline AudioSample Q15ToAudioSample(int16_t value) {
  return static_cast<AudioSample>(value) / kAudioPcmScale;
}

inline int16_t ComputeCombFeedbackQ15(uint32_t delaySamples, float t60Seconds) {
  const float exponent = -3.0f * static_cast<float>(delaySamples) /
                         (static_cast<float>(kSampleRateHz) * t60Seconds);
  float feedback = powf(10.0f, exponent);
  if (feedback > kMaxFeedback) {
    feedback = kMaxFeedback;
  }

  return static_cast<int16_t>(feedback * 32767.0f + 0.5f);
}

#ifdef ADV
inline __attribute__((section(".SDRAM1_NC"))) __attribute__((aligned(32)))
int16_t gReverbPredelayLeft[kPreDelaySamples] = {};
inline __attribute__((section(".SDRAM1_NC"))) __attribute__((aligned(32)))
int16_t gReverbPredelayRight[kPreDelaySamples] = {};
inline __attribute__((section(".SDRAM1_NC"))) __attribute__((aligned(32)))
int16_t gReverbCombStorageLeft[kCombStorageSizeLeft] = {};
inline __attribute__((section(".SDRAM1_NC"))) __attribute__((aligned(32)))
int16_t gReverbCombStorageRight[kCombStorageSizeRight] = {};
inline __attribute__((section(".SDRAM1_NC"))) __attribute__((aligned(32)))
int16_t gReverbAllpassStorageLeft[kAllpassStorageSizeLeft] = {};
inline __attribute__((section(".SDRAM1_NC"))) __attribute__((aligned(32)))
int16_t gReverbAllpassStorageRight[kAllpassStorageSizeRight] = {};
#else
inline int16_t gReverbPredelayLeft[kPreDelaySamples] = {};
inline int16_t gReverbPredelayRight[kPreDelaySamples] = {};
inline int16_t gReverbCombStorageLeft[kCombStorageSizeLeft] = {};
inline int16_t gReverbCombStorageRight[kCombStorageSizeRight] = {};
inline int16_t gReverbAllpassStorageLeft[kAllpassStorageSizeLeft] = {};
inline int16_t gReverbAllpassStorageRight[kAllpassStorageSizeRight] = {};
#endif

} // namespace freeverb_reverb_detail

class FreeverbReverb {
public:
  FreeverbReverb() { InitialiseFilters(); }

  void Reset() {
    NAssert(predelayLeft_ != nullptr);
    NAssert(predelayRight_ != nullptr);

    memset(predelayLeft_, 0,
           freeverb_reverb_detail::kPreDelaySamples * sizeof(int16_t));
    memset(predelayRight_, 0,
           freeverb_reverb_detail::kPreDelaySamples * sizeof(int16_t));
    predelayIndex_ = 0;

    for (int32_t filterIndex = 0; filterIndex < kCombFilterCount;
         ++filterIndex) {
      combLeft_[filterIndex].Reset();
      combRight_[filterIndex].Reset();
    }

    for (int32_t filterIndex = 0; filterIndex < kAllpassFilterCount;
         ++filterIndex) {
      allpassLeft_[filterIndex].Reset();
      allpassRight_[filterIndex].Reset();
    }
  }

  void Render(const AudioSample *input, AudioSample *output,
              int32_t sampleCount) {
    for (int32_t frameIndex = 0; frameIndex < sampleCount; ++frameIndex) {
      const int32_t sampleOffset = frameIndex * 2;
      const int16_t dryLeft =
          freeverb_reverb_detail::AudioSampleToQ15(input[sampleOffset]);
      const int16_t dryRight =
          freeverb_reverb_detail::AudioSampleToQ15(input[sampleOffset + 1]);

      const int16_t predelayedLeft = predelayLeft_[predelayIndex_];
      const int16_t predelayedRight = predelayRight_[predelayIndex_];
      predelayLeft_[predelayIndex_] = dryLeft;
      predelayRight_[predelayIndex_] = dryRight;
      ++predelayIndex_;
      if (predelayIndex_ >= freeverb_reverb_detail::kPreDelaySamples) {
        predelayIndex_ = 0;
      }

      int32_t wetLeftAccum = 0;
      int32_t wetRightAccum = 0;

      for (int32_t filterIndex = 0; filterIndex < kCombFilterCount;
           ++filterIndex) {
        wetLeftAccum += freeverb_reverb_detail::MultiplyQ15(
            combLeft_[filterIndex].Process(predelayedLeft),
            kCombGainQ15[filterIndex]);
        wetRightAccum += freeverb_reverb_detail::MultiplyQ15(
            combRight_[filterIndex].Process(predelayedRight),
            kCombGainQ15[filterIndex]);
      }

      int16_t wetLeft = freeverb_reverb_detail::SaturateQ15(wetLeftAccum);
      int16_t wetRight = freeverb_reverb_detail::SaturateQ15(wetRightAccum);

      for (int32_t filterIndex = 0; filterIndex < kAllpassFilterCount;
           ++filterIndex) {
        wetLeft = allpassLeft_[filterIndex].Process(wetLeft);
        wetRight = allpassRight_[filterIndex].Process(wetRight);
      }

      output[sampleOffset] = freeverb_reverb_detail::Q15ToAudioSample(
          freeverb_reverb_detail::ScaleQ15(
              freeverb_reverb_detail::MultiplyQ15(
                  wetLeft, freeverb_reverb_detail::kWetQ15),
              freeverb_reverb_detail::kMasterGainQ15));
      output[sampleOffset + 1] = freeverb_reverb_detail::Q15ToAudioSample(
          freeverb_reverb_detail::ScaleQ15(
              freeverb_reverb_detail::MultiplyQ15(
                  wetRight, freeverb_reverb_detail::kWetQ15),
              freeverb_reverb_detail::kMasterGainQ15));
    }
  }

  void SetParameters(float inputGain, float time, float diffusion,
                     float lowPass) {
    (void)inputGain;
    (void)time;
    (void)diffusion;
    (void)lowPass;
  }

private:
  struct CombFilter {
    void Setup(int16_t *buffer, uint32_t size, int16_t feedback) {
      NAssert(buffer != nullptr);
      NAssert(size > 0);

      buffer_ = buffer;
      size_ = size;
      feedback_ = feedback;
      Reset();
    }

    void Reset() {
      filter_ = 0;
      index_ = 0;

      if (buffer_ != nullptr && size_ > 0) {
        memset(buffer_, 0, static_cast<size_t>(size_) * sizeof(int16_t));
      }
    }

    int16_t Process(int16_t input) {
      const int16_t delayed = buffer_[index_];
      const int16_t feedbackSample =
          freeverb_reverb_detail::MultiplyQ15(feedback_, delayed);
      filter_ = freeverb_reverb_detail::SaturateQ15(
          freeverb_reverb_detail::MultiplyQ15(
              filter_, freeverb_reverb_detail::SaturateQ15(
                           freeverb_reverb_detail::kOneQ15 -
                           freeverb_reverb_detail::kDampQ15)) +
          freeverb_reverb_detail::MultiplyQ15(
              feedbackSample, freeverb_reverb_detail::kDampQ15));
      buffer_[index_] = freeverb_reverb_detail::SaturateQ15(
          static_cast<int32_t>(input) + filter_);

      ++index_;
      if (index_ >= size_) {
        index_ = 0;
      }

      return delayed;
    }

    int16_t *buffer_ = nullptr;
    uint32_t size_ = 0;
    uint32_t index_ = 0;
    int16_t feedback_ = 0;
    int16_t filter_ = 0;
  };

  struct AllpassFilter {
    void Setup(int16_t *buffer, uint32_t size, int16_t gain) {
      NAssert(buffer != nullptr);
      NAssert(size > 0);

      buffer_ = buffer;
      size_ = size;
      gain_ = gain;
      Reset();
    }

    void Reset() {
      index_ = 0;

      if (buffer_ != nullptr && size_ > 0) {
        memset(buffer_, 0, static_cast<size_t>(size_) * sizeof(int16_t));
      }
    }

    int16_t Process(int16_t input) {
      const int16_t delayed = buffer_[index_];
      const int16_t output = freeverb_reverb_detail::SaturateQ15(
          static_cast<int32_t>(delayed) -
          freeverb_reverb_detail::MultiplyQ15(input, gain_));
      buffer_[index_] = freeverb_reverb_detail::SaturateQ15(
          static_cast<int32_t>(input) +
          freeverb_reverb_detail::MultiplyQ15(output, gain_));

      ++index_;
      if (index_ >= size_) {
        index_ = 0;
      }

      return output;
    }

    int16_t *buffer_ = nullptr;
    uint32_t size_ = 0;
    uint32_t index_ = 0;
    int16_t gain_ = 0;
  };

  void InitialiseFilters() {
    predelayLeft_ = freeverb_reverb_detail::gReverbPredelayLeft;
    predelayRight_ = freeverb_reverb_detail::gReverbPredelayRight;

    uint32_t offset = 0;
    for (int32_t filterIndex = 0; filterIndex < kCombFilterCount;
         ++filterIndex) {
      combLeft_[filterIndex].Setup(
          &freeverb_reverb_detail::gReverbCombStorageLeft[offset],
          kCombDelayLeft[filterIndex],
          freeverb_reverb_detail::ComputeCombFeedbackQ15(
              kCombDelayLeft[filterIndex], kCombT60[filterIndex]));
      offset += kCombDelayLeft[filterIndex];
    }

    offset = 0;
    for (int32_t filterIndex = 0; filterIndex < kCombFilterCount;
         ++filterIndex) {
      combRight_[filterIndex].Setup(
          &freeverb_reverb_detail::gReverbCombStorageRight[offset],
          kCombDelayRight[filterIndex],
          freeverb_reverb_detail::ComputeCombFeedbackQ15(
              kCombDelayRight[filterIndex], kCombT60[filterIndex]));
      offset += kCombDelayRight[filterIndex];
    }

    offset = 0;
    for (int32_t filterIndex = 0; filterIndex < kAllpassFilterCount;
         ++filterIndex) {
      allpassLeft_[filterIndex].Setup(
          &freeverb_reverb_detail::gReverbAllpassStorageLeft[offset],
          kAllpassDelayLeft[filterIndex],
          freeverb_reverb_detail::kAllpassGainQ15);
      offset += kAllpassDelayLeft[filterIndex];
    }

    offset = 0;
    for (int32_t filterIndex = 0; filterIndex < kAllpassFilterCount;
         ++filterIndex) {
      allpassRight_[filterIndex].Setup(
          &freeverb_reverb_detail::gReverbAllpassStorageRight[offset],
          kAllpassDelayRight[filterIndex],
          freeverb_reverb_detail::kAllpassGainQ15);
      offset += kAllpassDelayRight[filterIndex];
    }

    Reset();
  }

  static constexpr int32_t kCombFilterCount = 8;
  static constexpr int32_t kAllpassFilterCount = 4;

  inline static constexpr uint32_t kCombDelayLeft[kCombFilterCount] = {
      509, 863, 1481, 2521, 4273, 7253, 10007, 15013};
  inline static constexpr uint32_t kCombDelayRight[kCombFilterCount] = {
      523, 877, 1489, 2531, 4283, 7283, 10037, 15031};
  inline static constexpr uint32_t kAllpassDelayLeft[kAllpassFilterCount] = {
      142, 396, 1071, 3079};
  inline static constexpr uint32_t kAllpassDelayRight[kAllpassFilterCount] = {
      145, 399, 1073, 3081};
  inline static constexpr float kCombT60[kCombFilterCount] = {
      0.25f, 0.30f, 0.40f, 0.80f, 2.00f, 6.00f, 10.00f, 20.00f};

  inline static constexpr int16_t kCombGainQ15[kCombFilterCount] = {
      static_cast<int16_t>(0.62f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.60f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.58f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.55f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.52f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.50f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.48f * 32767.0f + 0.5f),
      static_cast<int16_t>(0.45f * 32767.0f + 0.5f),
  };

  CombFilter combLeft_[kCombFilterCount];
  CombFilter combRight_[kCombFilterCount];
  AllpassFilter allpassLeft_[kAllpassFilterCount];
  AllpassFilter allpassRight_[kAllpassFilterCount];

  int16_t *predelayLeft_ = nullptr;
  int16_t *predelayRight_ = nullptr;
  uint32_t predelayIndex_ = 0;
};

#endif
