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

#include "DattorroReverb.h"

#include "System/Console/n_assert.h"
#include "config/MemorySections.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace {

// The signal flow, constants, and parameter semantics below are translated
// from Khoi Nguyen's DattorroReverbNode, whose author placed the implementation
// in the public domain with a no-liability notice:
// https://github.com/khoin/DattorroReverbNode
constexpr double kExcursionAngularScales[2] = {6.2800, 6.2847};
constexpr double kWetOutputScale = 0.6;
constexpr uint32_t kPreDelayCapacity = 44160;
constexpr float kMaximumPreDelayMilliseconds = 1000.0f;
constexpr float kMinimumBandwidth = 0.02f;
constexpr float kDefaultBandwidth = 0.9999f;
constexpr float kDefaultDamping = 0.005f;
constexpr float kMaximumDamping = 0.98f;
constexpr float kMinimumDecayMilliseconds = 100.0f;
constexpr float kDefaultDecayMilliseconds = 3600.0f;
constexpr float kMaximumDecayMilliseconds = 30000.0f;
constexpr float kMaximumModulationRateHz = 2.0f;
constexpr float kMaximumModulationDepthMilliseconds = 2.0f;
constexpr float kDefaultModulationRateHz = 0.5f;
constexpr float kDefaultModulationDepthMilliseconds = 0.7f;
constexpr float kDecayTimeConstantMilliseconds = 2495.3299f;
constexpr float kTailMarginMilliseconds = 1000.0f;

// DattorroReverbNode rounds the delays from seconds to the current sample rate,
// then gives every delay a power-of-two buffer for masked indexing.
constexpr uint32_t kDelayLengths[12] = {210,  159,  562,  410,  996,  6598,
                                        2667, 5512, 1345, 6249, 3936, 4687};
constexpr uint32_t kDelayCapacities[12] = {
    256, 256, 1024, 512, 1024, 8192, 4096, 8192, 2048, 8192, 4096, 8192,
};
constexpr uint32_t kOutputTaps[14] = {394, 4407, 2835, 2958, 2949, 277, 1580,
                                      523, 5375, 1820, 3961, 3128, 496, 179};

constexpr uint32_t kDelayFloatCount =
    kDelayCapacities[0] + kDelayCapacities[1] + kDelayCapacities[2] +
    kDelayCapacities[3] + kDelayCapacities[4] + kDelayCapacities[5] +
    kDelayCapacities[6] + kDelayCapacities[7] + kDelayCapacities[8] +
    kDelayCapacities[9] + kDelayCapacities[10] + kDelayCapacities[11];
static_assert(kDelayFloatCount == DattorroReverb::kInternalFloatCount);
static_assert(kPreDelayCapacity == DattorroReverb::kExternalFloatCount);

float Clamp(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

int32_t ClampParameter(int32_t value) {
  if (value < DattorroReverbSettings::MinimumParameterValue) {
    return DattorroReverbSettings::MinimumParameterValue;
  }
  if (value > DattorroReverbSettings::MaximumParameterValue) {
    return DattorroReverbSettings::MaximumParameterValue;
  }
  return value;
}

float MapPreDelayMilliseconds(int32_t value) {
  return static_cast<float>(ClampParameter(value)) *
         kMaximumPreDelayMilliseconds /
         static_cast<float>(DattorroReverbSettings::MaximumParameterValue);
}

float MapInputBandwidth(int32_t value) {
  const float tone =
      static_cast<float>(ClampParameter(value)) /
      static_cast<float>(DattorroReverbSettings::MaximumParameterValue);
  return kMinimumBandwidth + (kDefaultBandwidth - kMinimumBandwidth) * tone;
}

float MapDiffusion(int32_t value) {
  return static_cast<float>(value < DattorroReverbSettings::MinimumDiffusion
                                ? DattorroReverbSettings::MinimumDiffusion
                            : value > DattorroReverbSettings::MaximumDiffusion
                                ? DattorroReverbSettings::MaximumDiffusion
                                : value) /
         static_cast<float>(DattorroReverbSettings::DiffusionScale);
}

float MapDecayMilliseconds(int32_t value) {
  value = ClampParameter(value);
  if (value > DattorroReverbSettings::MaximumFiniteDecayValue) {
    value = DattorroReverbSettings::MaximumFiniteDecayValue;
  }

  // Keep the original 3.6-second sound at the default byte while providing
  // finer resolution for short decays and the full 30-second range above it.
  if (value <= DattorroReverbSettings::DefaultDecay) {
    return kMinimumDecayMilliseconds +
           static_cast<float>(value) *
               (kDefaultDecayMilliseconds - kMinimumDecayMilliseconds) /
               static_cast<float>(DattorroReverbSettings::DefaultDecay);
  }

  return kDefaultDecayMilliseconds +
         static_cast<float>(value - DattorroReverbSettings::DefaultDecay) *
             (kMaximumDecayMilliseconds - kDefaultDecayMilliseconds) /
             static_cast<float>(
                 DattorroReverbSettings::MaximumFiniteDecayValue -
                 DattorroReverbSettings::DefaultDecay);
}

float MapTailDampingCoefficient(int32_t value) {
  const float tone =
      static_cast<float>(ClampParameter(value)) /
      static_cast<float>(DattorroReverbSettings::MaximumParameterValue);
  const float damping =
      kDefaultDamping + (kMaximumDamping - kDefaultDamping) * (1.0f - tone);
  return 1.0f - damping;
}

float MapModulationRate(int32_t value) {
  value = ClampParameter(value);
  if (value <= DattorroReverbSettings::DefaultModulationRate) {
    return static_cast<float>(value) * kDefaultModulationRateHz /
           static_cast<float>(DattorroReverbSettings::DefaultModulationRate);
  }

  return kDefaultModulationRateHz +
         static_cast<float>(value -
                            DattorroReverbSettings::DefaultModulationRate) *
             (kMaximumModulationRateHz - kDefaultModulationRateHz) /
             static_cast<float>(DattorroReverbSettings::MaximumParameterValue -
                                DattorroReverbSettings::DefaultModulationRate);
}

float MapModulationDepth(int32_t value) {
  value = ClampParameter(value);
  if (value <= DattorroReverbSettings::DefaultModulationDepth) {
    return static_cast<float>(value) * kDefaultModulationDepthMilliseconds /
           static_cast<float>(DattorroReverbSettings::DefaultModulationDepth);
  }

  return kDefaultModulationDepthMilliseconds +
         static_cast<float>(value -
                            DattorroReverbSettings::DefaultModulationDepth) *
             (kMaximumModulationDepthMilliseconds -
              kDefaultModulationDepthMilliseconds) /
             static_cast<float>(DattorroReverbSettings::MaximumParameterValue -
                                DattorroReverbSettings::DefaultModulationDepth);
}

} // namespace

DattorroReverb::DattorroReverb() { Configure(DattorroReverbSettings{}); }

void DattorroReverb::Configure(const DattorroReverbSettings &settings) {
  const float preDelayMilliseconds =
      MapPreDelayMilliseconds(settings.preDelay_);
  const float preDelaySamples =
      preDelayMilliseconds * static_cast<float>(kSampleRateHz) / 1000.0f;
  const int32_t decayValue = ClampParameter(settings.decay_);
  const bool hold = decayValue == DattorroReverbSettings::MaximumParameterValue;
  const float decayMilliseconds = MapDecayMilliseconds(decayValue);
  const float decay =
      hold ? 1.0f : expf(-kDecayTimeConstantMilliseconds / decayMilliseconds);
  const float excursionRate = MapModulationRate(settings.modulationRate_);
  const float excursionDepth = MapModulationDepth(settings.modulationDepth_);
  double angularIncrements[2];
  for (uint32_t index = 0; index < 2; ++index) {
    angularIncrements[index] =
        static_cast<double>(excursionRate) * kExcursionAngularScales[index] /
        static_cast<double>(DattorroReverb::kSampleRateHz);
  }

  parameters_ = {
      .preDelaySamples_ = static_cast<uint32_t>(
          Clamp(preDelaySamples, 0.0f,
                static_cast<float>(DattorroReverb::kSampleRateHz - 1))),
      .bandwidth_ = MapInputBandwidth(settings.inputTone_),
      .inputDiffusion1_ = MapDiffusion(settings.preDiffusion1_),
      .inputDiffusion2_ = MapDiffusion(settings.preDiffusion2_),
      .decay_ = decay,
      .decayDiffusion1_ = MapDiffusion(settings.decayDiffusion_),
      // Dattorro recommends following decay with this coefficient while
      // keeping it between 0.25 and 0.5.
      .decayDiffusion2_ = Clamp(decay + 0.15f, 0.25f, 0.5f),
      .dampingCoefficient_ = MapTailDampingCoefficient(settings.tailTone_),
      .excursionDepthSamples_ =
          static_cast<double>(excursionDepth) *
          static_cast<double>(DattorroReverb::kSampleRateHz) / 1000.0,
      .excursionCosineIncrement_ = {cos(angularIncrements[0]),
                                    cos(angularIncrements[1])},
      .excursionSineIncrement_ = {sin(angularIncrements[0]),
                                  sin(angularIncrements[1])},
  };

  if (hold) {
    tailFrameCount_ = std::numeric_limits<int32_t>::max();
  } else {
    const double tailMilliseconds = static_cast<double>(preDelayMilliseconds) +
                                    decayMilliseconds + kTailMarginMilliseconds;
    tailFrameCount_ = static_cast<int32_t>(
        ceil(tailMilliseconds * static_cast<double>(kSampleRateHz) / 1000.0));
  }
}

bool DattorroReverb::Initialize(uint8_t *internalMemory,
                                size_t internalMemorySize,
                                uint8_t *externalMemory,
                                size_t externalMemorySize) {
  if (internalMemory == nullptr || internalMemorySize < kInternalMemorySize ||
      externalMemory == nullptr || externalMemorySize < kExternalMemorySize ||
      reinterpret_cast<uintptr_t>(internalMemory) % alignof(float) != 0 ||
      reinterpret_cast<uintptr_t>(externalMemory) % alignof(float) != 0) {
    return false;
  }

  internalMemory_ = reinterpret_cast<float *>(internalMemory);
  externalMemory_ = reinterpret_cast<float *>(externalMemory);

  uint32_t internalOffset = 0;
  preDelay_.Initialize(externalMemory_, kPreDelayCapacity);
  for (uint32_t index = 0; index < 12; ++index) {
    delays_[index].Initialize(internalMemory_ + internalOffset,
                              kDelayCapacities[index], kDelayLengths[index]);
    internalOffset += kDelayCapacities[index];
  }
  NAssert(internalOffset == kInternalFloatCount);

  initialized_ = true;
  Reset();
  return true;
}

void DattorroReverb::Deinitialize() {
  internalMemory_ = nullptr;
  externalMemory_ = nullptr;
  initialized_ = false;
}

void DattorroReverb::Reset() {
  if (!initialized_) {
    return;
  }

  memset(internalMemory_, 0, kInternalMemorySize);
  memset(externalMemory_, 0, kExternalMemorySize);
  preDelay_.Reset();
  for (DelayLine &delay : delays_) {
    delay.Reset();
  }
  for (Oscillator &oscillator : excursionOscillators_) {
    oscillator.Reset();
  }
  inputLowPass_ = 0.0;
  tankLowPass_[0] = 0.0;
  tankLowPass_[1] = 0.0;
}

void DattorroReverb::PreDelay::Initialize(float *buffer, uint32_t capacity) {
  buffer_ = buffer;
  capacity_ = capacity;
  writeIndex_ = 0;
}

void DattorroReverb::PreDelay::Reset() { writeIndex_ = 0; }

double DattorroReverb::PreDelay::Process(double input, uint32_t delaySamples) {
  NAssert(buffer_ != nullptr);
  NAssert(delaySamples < capacity_);
  buffer_[writeIndex_] = static_cast<float>(input);
  const uint32_t readIndex =
      (capacity_ + writeIndex_ - delaySamples) % capacity_;
  const double output = buffer_[readIndex];
  ++writeIndex_;
  if (writeIndex_ == capacity_) {
    writeIndex_ = 0;
  }
  return output;
}

void DattorroReverb::DelayLine::Initialize(float *buffer, uint32_t capacity,
                                           uint32_t delayLength) {
  NAssert(buffer != nullptr);
  NAssert(capacity > 0 && (capacity & (capacity - 1)) == 0);
  NAssert(delayLength > 0 && delayLength <= capacity);
  buffer_ = buffer;
  capacity_ = capacity;
  mask_ = capacity - 1;
  initialWriteIndex_ = delayLength - 1;
  Reset();
}

void DattorroReverb::DelayLine::Reset() {
  writeIndex_ = initialWriteIndex_;
  readIndex_ = 0;
}

double DattorroReverb::DelayLine::Write(double value) {
  // A Javascript typed-array assignment rounds the stored sample to float but
  // the assignment expression itself evaluates to the original Number value.
  buffer_[writeIndex_] = static_cast<float>(value);
  return value;
}

double DattorroReverb::DelayLine::Read() const { return buffer_[readIndex_]; }

double DattorroReverb::DelayLine::ReadAt(uint32_t offset) const {
  return buffer_[(readIndex_ + offset) & mask_];
}

double DattorroReverb::DelayLine::ReadCubicAt(double offset) const {
  const uint32_t integerOffset = static_cast<uint32_t>(offset);
  const double fraction = offset - static_cast<double>(integerOffset);
  uint32_t index = (readIndex_ + integerOffset - 1) & mask_;
  const double x0 = buffer_[index];
  index = (index + 1) & mask_;
  const double x1 = buffer_[index];
  index = (index + 1) & mask_;
  const double x2 = buffer_[index];
  index = (index + 1) & mask_;
  const double x3 = buffer_[index];

  const double a = (3.0 * (x1 - x2) - x0 + x3) * 0.5;
  const double b = 2.0 * x2 + x0 - (5.0 * x1 + x3) * 0.5;
  const double c = (x2 - x0) * 0.5;
  return ((a * fraction + b) * fraction + c) * fraction + x1;
}

void DattorroReverb::DelayLine::Advance() {
  writeIndex_ = (writeIndex_ + 1) & mask_;
  readIndex_ = (readIndex_ + 1) & mask_;
}

void DattorroReverb::Oscillator::Reset() {
  cosine_ = 1.0;
  sine_ = 0.0;
  samplesUntilNormalization_ = 4096;
}

void DattorroReverb::Oscillator::Advance(double cosineIncrement,
                                         double sineIncrement) {
  const double previousCosine = cosine_;
  cosine_ = previousCosine * cosineIncrement - sine_ * sineIncrement;
  sine_ = sine_ * cosineIncrement + previousCosine * sineIncrement;

  --samplesUntilNormalization_;
  if (samplesUntilNormalization_ == 0) {
    // The rotation is mathematically unit length. Occasional normalization
    // prevents floating-point drift without evaluating trigonometry per frame.
    const double inverseMagnitude =
        1.0 / sqrt(cosine_ * cosine_ + sine_ * sine_);
    cosine_ *= inverseMagnitude;
    sine_ *= inverseMagnitude;
    samplesUntilNormalization_ = 4096;
  }
}

void DattorroReverb::AdvanceDelayLines() {
  for (DelayLine &delay : delays_) {
    delay.Advance();
  }
}

PICOTRACKER_FAST_CODE("dattorro_reverb_render")
void DattorroReverb::Render(const AudioSample *input, AudioSample *output,
                            int32_t sampleCount) {
  NAssert(initialized_);
  NAssert(input != nullptr);
  NAssert(output != nullptr);
  NAssert(sampleCount >= 0);

  for (int32_t frame = 0; frame < sampleCount; ++frame) {
    const int32_t sample = frame * 2;
    const double leftInput = input[sample];
    const double rightInput = input[sample + 1];
    const double monoInput = (leftInput + rightInput) * 0.5;
    const double preDelayed =
        preDelay_.Process(monoInput, parameters_.preDelaySamples_);
    inputLowPass_ += parameters_.bandwidth_ * (preDelayed - inputLowPass_);

    // Four serial allpasses diffuse the mono input before it enters both
    // halves of the figure-eight tank.
    double pre = delays_[0].Write(inputLowPass_ - parameters_.inputDiffusion1_ *
                                                      delays_[0].Read());
    pre = delays_[1].Write(parameters_.inputDiffusion1_ *
                               (pre - delays_[1].Read()) +
                           delays_[0].Read());
    pre = delays_[2].Write(parameters_.inputDiffusion1_ * pre +
                           delays_[1].Read() -
                           parameters_.inputDiffusion2_ * delays_[2].Read());
    pre = delays_[3].Write(parameters_.inputDiffusion2_ *
                               (pre - delays_[3].Read()) +
                           delays_[2].Read());
    const double split = parameters_.inputDiffusion2_ * pre + delays_[3].Read();

    // The source implementation uses one unbounded phase with slightly
    // different angular constants. Two rotating unit vectors produce the same
    // frequencies while retaining precision during long-running playback.
    const double excursionLeft = parameters_.excursionDepthSamples_ *
                                 (1.0 + excursionOscillators_[0].cosine_);
    const double excursionRight = parameters_.excursionDepthSamples_ *
                                  (1.0 + excursionOscillators_[1].sine_);

    const double leftDiffuser = delays_[4].ReadCubicAt(excursionLeft);
    double temporary =
        delays_[4].Write(split + parameters_.decay_ * delays_[11].Read() +
                         parameters_.decayDiffusion1_ * leftDiffuser);
    delays_[5].Write(leftDiffuser - parameters_.decayDiffusion1_ * temporary);
    tankLowPass_[0] +=
        parameters_.dampingCoefficient_ * (delays_[5].Read() - tankLowPass_[0]);
    temporary =
        delays_[6].Write(parameters_.decay_ * tankLowPass_[0] -
                         parameters_.decayDiffusion2_ * delays_[6].Read());
    delays_[7].Write(delays_[6].Read() +
                     parameters_.decayDiffusion2_ * temporary);

    const double rightDiffuser = delays_[8].ReadCubicAt(excursionRight);
    temporary =
        delays_[8].Write(split + parameters_.decay_ * delays_[7].Read() +
                         parameters_.decayDiffusion1_ * rightDiffuser);
    delays_[9].Write(rightDiffuser - parameters_.decayDiffusion1_ * temporary);
    tankLowPass_[1] +=
        parameters_.dampingCoefficient_ * (delays_[9].Read() - tankLowPass_[1]);
    temporary =
        delays_[10].Write(parameters_.decay_ * tankLowPass_[1] -
                          parameters_.decayDiffusion2_ * delays_[10].Read());
    delays_[11].Write(delays_[10].Read() +
                      parameters_.decayDiffusion2_ * temporary);

    const double leftWet =
        delays_[9].ReadAt(kOutputTaps[0]) + delays_[9].ReadAt(kOutputTaps[1]) -
        delays_[10].ReadAt(kOutputTaps[2]) +
        delays_[11].ReadAt(kOutputTaps[3]) - delays_[5].ReadAt(kOutputTaps[4]) -
        delays_[6].ReadAt(kOutputTaps[5]) - delays_[7].ReadAt(kOutputTaps[6]);
    const double rightWet =
        delays_[5].ReadAt(kOutputTaps[7]) + delays_[5].ReadAt(kOutputTaps[8]) -
        delays_[6].ReadAt(kOutputTaps[9]) + delays_[7].ReadAt(kOutputTaps[10]) -
        delays_[9].ReadAt(kOutputTaps[11]) -
        delays_[10].ReadAt(kOutputTaps[12]) -
        delays_[11].ReadAt(kOutputTaps[13]);

    // Advance mixes effect returns with the dry master signal outside the
    // algorithm. A fixed wet=1 keeps the source implementation's 0.6 wet-path
    // normalization without adding a second, misleading mix control.
    output[sample] = static_cast<AudioSample>(leftWet * kWetOutputScale);
    output[sample + 1] = static_cast<AudioSample>(rightWet * kWetOutputScale);

    excursionOscillators_[0].Advance(parameters_.excursionCosineIncrement_[0],
                                     parameters_.excursionSineIncrement_[0]);
    excursionOscillators_[1].Advance(parameters_.excursionCosineIncrement_[1],
                                     parameters_.excursionSineIncrement_[1]);
    AdvanceDelayLines();
  }
}
