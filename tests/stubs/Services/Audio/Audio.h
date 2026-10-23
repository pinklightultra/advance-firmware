/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

/*
 * Test stub for host-only builds.
 */
#ifndef _AUDIO_H_
#define _AUDIO_H_

class Audio {
public:
  static Audio *GetInstance() {
    static Audio instance;
    return &instance;
  }
  int GetSampleRate() { return 44100; }
};

#endif
