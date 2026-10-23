/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _AUDIO_SAMPLE_H_
#define _AUDIO_SAMPLE_H_

using AudioSample = float;

constexpr AudioSample kAudioSampleZero = 0.0f;
constexpr AudioSample kAudioSampleOne = 1.0f;
constexpr AudioSample kAudioSampleMin = -1.0f;
constexpr AudioSample kAudioSampleMax = 32767.0f / 32768.0f;
constexpr AudioSample kAudioPcmScale = 32768.0f;

#endif
