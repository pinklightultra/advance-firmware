/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _RINGS_REVERB_IMPL_H_
#define _RINGS_REVERB_IMPL_H_

#include "Application/Audio/AudioSample.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif
#include "rings/dsp/fx/reverb.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cstdint>

namespace rings_reverb_impl_detail {

#ifdef ADV
inline __attribute__((section(".RAM_D2")))
__attribute__((aligned(32))) float gRingsBuffer[32768] = {};
#else
inline float gRingsBuffer[32768] = {};
#endif

} // namespace rings_reverb_impl_detail

class RingsReverb {
public:
  RingsReverb() { Reset(); }

  void Reset() {
    reverb_.Init(rings_reverb_impl_detail::gRingsBuffer);
    reverb_.set_input_gain(0.2f);
    reverb_.set_time(0.35f);
    reverb_.set_diffusion(0.625f);
    reverb_.set_lp(0.7f);
  }

  void SetParameters(float inputGain, float time, float diffusion,
                     float lowPass) {
    reverb_.set_input_gain(inputGain);
    reverb_.set_time(time);
    reverb_.set_diffusion(diffusion);
    reverb_.set_lp(lowPass);
  }

  void Render(const AudioSample *input, AudioSample *output,
              int32_t sampleCount) {
    reverb_.Process(input, output, static_cast<size_t>(sampleCount));
  }

private:
  rings::Reverb reverb_;
};

#endif
