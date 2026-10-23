/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "AudioMixer.h"
#include "System/Console/Trace.h"
#include "System/Profiler/DWTProfiler.h"
#include "System/System/System.h"
#include <algorithm>

namespace {

constexpr int kEngineSampleRate = 44100;

DWTProfileStats gAudioMixerRenderProfile = {"mixer total", true};

void registerAudioMixerProfiles() {
  static bool registered = false;
  if (registered) {
    return;
  }

  RegisterDWTProfileStats(&gAudioMixerRenderProfile);
  registered = true;
}

} // namespace

AudioSample AudioMixer::renderBuffer_[MAX_SAMPLE_COUNT * 2];

AudioMixer::AudioMixer(const char *name)
    : enableRendering_(0), name_(name), modules_() {
  registerAudioMixerProfiles();
  volume_ = kAudioSampleOne;
  renderGain_ = kAudioSampleOne;
};

AudioMixer::~AudioMixer() {}

void AudioMixer::SetFileRenderer(const char *path) { renderPath_ = path; };

void AudioMixer::EnableRendering(bool enable) {
  if (enable == enableRendering_) {
    return;
  }

  if (enable) {
    if (!writer_.Open(renderPath_.c_str())) {
      Trace::Error("AUDIO_MIXER", "Failed to open render file: %s",
                   renderPath_.c_str());
      enableRendering_ = false;
      return;
    }
  }

  enableRendering_ = enable;
  if (!enable) {
    writer_.Close();
  }
};

bool AudioMixer::Render(AudioSample *buffer, int samplecount) {
  ScopedDWTProfile instanceProfile(profileStats_, samplecount,
                                   kEngineSampleRate,
                                   static_cast<int>(profileSlotCount_));
  AccumulatingDWTProfile mixProfile(
      profileMixMath_ ? &gAudioMixerRenderProfile : nullptr, samplecount,
      kEngineSampleRate, static_cast<int>(profileSlotCount_));
  bool gotData = false;
  AudioSample peakL = 0;
  AudioSample peakR = 0;

  for (auto *mod : modules_) {
    if (!mod) {
      continue;
    }
    if (!gotData) {
      gotData = mod->Render(buffer, samplecount);
    } else {
      if (mod->Render(renderBuffer_, samplecount)) {
        AudioSample *dst = buffer;
        AudioSample *src = renderBuffer_;
        int count = samplecount * 2;
        mixProfile.Begin();

        /* Manually unrolling the loop gives a 25% performance increase (from
         * 6500 cycles for 800 samples to 5100 cycles. This is due to being able
         * to schedule independent loads simultanously, reduce load latency by
         * loading independent loads instead of immediately executing a
         * dependent action (add) having to wait for the load to complete.
         * Potentially also reducing the load comparison overhead 16
         * instructions in the unroll gives the best performance, 8 gave ~17%
         * improvement.
         */
        int i = 0;
        for (; i + 16 <= count; i += 16) {
          dst[i + 0] += src[i + 0];
          dst[i + 1] += src[i + 1];
          dst[i + 2] += src[i + 2];
          dst[i + 3] += src[i + 3];
          dst[i + 4] += src[i + 4];
          dst[i + 5] += src[i + 5];
          dst[i + 6] += src[i + 6];
          dst[i + 7] += src[i + 7];
          dst[i + 8] += src[i + 8];
          dst[i + 9] += src[i + 9];
          dst[i + 10] += src[i + 10];
          dst[i + 11] += src[i + 11];
          dst[i + 12] += src[i + 12];
          dst[i + 13] += src[i + 13];
          dst[i + 14] += src[i + 14];
          dst[i + 15] += src[i + 15];
        }
        for (; i < count; ++i)
          dst[i] += src[i];
        mixProfile.End();
      }
    }
  }

  // Apply volume to mix of all of this instance's "sub" audiomixers
  // TODO (democloid): This is wildly inefficient, doing this loop takes 4 - 5
  // times the time it takes a mix loop above. Some tests show that at least
  // double performance is not hard to achieve
  if (gotData) {
    mixProfile.Begin();
    AudioSample *c = buffer;

    if (volume_ == kAudioSampleOne) {
      // unity gain, no calculations to be done, just grab the levels
      for (int i = 0; i < samplecount; i += 32, c += 64) {
        AudioSample r = *c;
        AudioSample l = *(c + 1);
        if (r > peakR)
          peakR = r;
        if (l > peakL)
          peakL = l;
      }
    } else {
      for (int i = 0; i < samplecount; i++) {
        // Right
        AudioSample r = *c * volume_;
        *c++ = r;

        // Left
        AudioSample l = *c * volume_;
        *c++ = l;

        // update the level every 32 sample pairs
        // (!(i & 31)) =^= (i % 32 == 0) and is used for performance
        if (!(i & 31)) {
          if (r > peakR)
            peakR = r;
          if (l > peakL)
            peakL = l;
        }
      }
    }
    mixProfile.End();
  }

  // Always update peakMixerLevel_ regardless of whether we got data
  // This ensures VU meters update properly in all scenarios
  peakMixerLevel_ = static_cast<int>(std::clamp(peakL, 0.0f, kAudioSampleMax) *
                                     kAudioPcmScale)
                        << 16 |
                    static_cast<int>(std::clamp(peakR, 0.0f, kAudioSampleMax) *
                                     kAudioPcmScale);

  if (enableRendering_ && writer_.IsOpen()) {
    if (!gotData) {
      memset(buffer, 0, samplecount * 2 * sizeof(AudioSample));
    }

    AudioSample *renderSource = buffer;
    if (renderGain_ != kAudioSampleOne) {
      const int sampleTotal = samplecount * 2;
      memcpy(renderBuffer_, buffer, sampleTotal * sizeof(AudioSample));
      for (int i = 0; i < sampleTotal; ++i) {
        renderBuffer_[i] *= renderGain_;
      }
      renderSource = renderBuffer_;
    }

    writer_.AddBuffer(renderSource, samplecount);
  }
  return gotData;
};

void AudioMixer::SetVolume(AudioSample volume) { volume_ = volume; }

void AudioMixer::SetRenderGain(AudioSample gain) { renderGain_ = gain; }

void AudioMixer::AddModule(AudioModule &module) {
  if (modules_.full()) {
    Trace::Error("AUDIOMIXER", "Module list full");
    return;
  }
  modules_.push_back(&module);
}

void AudioMixer::RemoveModule(AudioModule &module) {
  for (auto it = modules_.begin(); it != modules_.end(); ++it) {
    if (*it == &module) {
      modules_.erase(it);
      return;
    }
  }
}

void AudioMixer::ClearModules() { modules_.clear(); }
