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

#ifndef _AUDIO_MODULE_H_
#define _AUDIO_MODULE_H_

#include "Application/Audio/AudioSample.h"
#include "Foundation/Types/Types.h"

class AudioModule {
public:
  virtual ~AudioModule(){};
  virtual bool Render(AudioSample *buffer, int samplecount) = 0;
};

#endif
