/*
 * SPDX-License-Identifier: LicenseRef-Dattorro-PD AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Adapted from Khoi Nguyen's DattorroReverbNode, released into the public
 * domain. Original terms:
 * LICENSES/LicenseRef-Dattorro-PD.txt. New original
 * contributions by xiphonics, inc. are GPL-3.0-only. Existing grants are
 * preserved. See LICENSING.md.
 */

#ifndef _DATTORRO_REVERB_H_
#define _DATTORRO_REVERB_H_

#include "Application/Audio/AudioSample.h"

#include <cstddef>
#include <cstdint>

// These are the values stored by the project and edited by the Master Effects
// screen. The main controls use the same 00-FF values accepted by tracker
// commands; the three detailed diffusion controls use thousandths.
struct DattorroReverbSettings {
  static constexpr int32_t MinimumParameterValue = 0x00;
  static constexpr int32_t MaximumParameterValue = 0xFF;
  static constexpr int32_t MaximumFiniteDecayValue = 0xFE;
  static constexpr int32_t DefaultPreDelay = 0x00;
  static constexpr int32_t DefaultInputTone = 0xFF;
  static constexpr int32_t DefaultDecay = 0x1E;
  static constexpr int32_t DefaultTailTone = 0xFF;
  static constexpr int32_t DefaultModulationRate = 0x40;
  static constexpr int32_t DefaultModulationDepth = 0x5A;
  static constexpr int32_t MinimumDiffusion = 0;
  static constexpr int32_t MaximumDiffusion = 999;
  static constexpr int32_t DiffusionScale = 1000;
  static constexpr int32_t DefaultPreDiffusion1 = 750;
  static constexpr int32_t DefaultPreDiffusion2 = 625;
  static constexpr int32_t DefaultDecayDiffusion = 700;

  int32_t preDelay_ = DefaultPreDelay;
  int32_t inputTone_ = DefaultInputTone;
  int32_t preDiffusion1_ = DefaultPreDiffusion1;
  int32_t preDiffusion2_ = DefaultPreDiffusion2;
  int32_t decay_ = DefaultDecay;
  int32_t decayDiffusion_ = DefaultDecayDiffusion;
  int32_t tailTone_ = DefaultTailTone;
  int32_t modulationRate_ = DefaultModulationRate;
  int32_t modulationDepth_ = DefaultModulationDepth;
};

class DattorroReverb {
public:
  static constexpr uint32_t kSampleRateHz = 44100;

  // Every diffusion and tank delay stays in internal D2 RAM. Only the
  // sequential one-second pre-delay uses cacheable SDRAM.
  static constexpr uint32_t kInternalFloatCount = 46080;
  static constexpr uint32_t kExternalFloatCount = 44160;
  static constexpr size_t kInternalMemorySize =
      kInternalFloatCount * sizeof(float);
  static constexpr size_t kExternalMemorySize =
      kExternalFloatCount * sizeof(float);

  DattorroReverb();

  bool Initialize(uint8_t *internalMemory, size_t internalMemorySize,
                  uint8_t *externalMemory, size_t externalMemorySize);
  void Deinitialize();
  void Reset();
  void Configure(const DattorroReverbSettings &settings);
  void Render(const AudioSample *input, AudioSample *output,
              int32_t sampleCount);
  bool IsInitialized() const { return initialized_; }
  int32_t GetTailFrameCount() const { return tailFrameCount_; }

private:
  struct RenderParameters {
    uint32_t preDelaySamples_;
    float bandwidth_;
    float inputDiffusion1_;
    float inputDiffusion2_;
    float decay_;
    float decayDiffusion1_;
    float decayDiffusion2_;
    double dampingCoefficient_;
    double excursionDepthSamples_;
    double excursionCosineIncrement_[2];
    double excursionSineIncrement_[2];
  };

  struct PreDelay {
    void Initialize(float *buffer, uint32_t capacity);
    void Reset();
    double Process(double input, uint32_t delaySamples);

    float *buffer_ = nullptr;
    uint32_t capacity_ = 0;
    uint32_t writeIndex_ = 0;
  };

  struct DelayLine {
    void Initialize(float *buffer, uint32_t capacity, uint32_t delayLength);
    void Reset();
    double Write(double value);
    double Read() const;
    double ReadAt(uint32_t offset) const;
    double ReadCubicAt(double offset) const;
    void Advance();

    float *buffer_ = nullptr;
    uint32_t capacity_ = 0;
    uint32_t mask_ = 0;
    uint32_t initialWriteIndex_ = 0;
    uint32_t writeIndex_ = 0;
    uint32_t readIndex_ = 0;
  };

  struct Oscillator {
    void Reset();
    void Advance(double cosineIncrement, double sineIncrement);

    double cosine_ = 1.0;
    double sine_ = 0.0;
    uint32_t samplesUntilNormalization_ = 4096;
  };

  void AdvanceDelayLines();

  float *internalMemory_ = nullptr;
  float *externalMemory_ = nullptr;
  RenderParameters parameters_{};
  int32_t tailFrameCount_ = 0;
  PreDelay preDelay_;
  DelayLine delays_[12];
  Oscillator excursionOscillators_[2];
  // Javascript keeps processing state in double-precision Number values even
  // though its typed delay buffers round every stored sample to float.
  double inputLowPass_ = 0.0;
  double tankLowPass_[2] = {0.0, 0.0};
  bool initialized_ = false;
};

#endif
