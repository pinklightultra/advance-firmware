/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _RECORD_STREAMER_H_
#define _RECORD_STREAMER_H_

#include "Application/Audio/AudioSample.h"
#include "Services/Audio/AudioModule.h"

enum RecordStreamerMode { RSM_STOPPED, RSM_PLAYING };

class RecordStreamer : public AudioModule {
public:
  RecordStreamer();
  virtual ~RecordStreamer();
  virtual bool Render(AudioSample *buffer, int samplecount);
  bool Start(uint16_t *srcBuffer, uint32_t size, bool stereo);
  void Stop();
  bool IsPlaying();

protected:
  uint16_t *srcBuffer_;
  uint32_t bufferSize_;
  bool stereo_;
  RecordStreamerMode mode_;
  uint32_t srcBufferPos_;
};

#endif
