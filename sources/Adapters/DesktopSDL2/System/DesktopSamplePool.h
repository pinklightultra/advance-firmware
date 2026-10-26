/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * DesktopSamplePool.h - SamplePool for picoTracker Advance on Desktop
 */

#ifndef _DESKTOP_SAMPLE_POOL_H_
#define _DESKTOP_SAMPLE_POOL_H_

#include "Application/Instruments/SamplePool.h"
#include <stdint.h>

#define DESKTOP_STORE1_SIZE (32 * 1024 * 1024)
#define DESKTOP_STORE2_SIZE (32 * 1024 * 1024)

class DesktopSamplePool : public SamplePool {
public:
  DesktopSamplePool();
  virtual ~DesktopSamplePool();

  virtual void Reset() override;
  virtual bool CheckSampleFits(uint32_t sampleSize) override;
  virtual uint32_t GetAvailableSampleStorageSpace() override;
  virtual bool unloadSample(uint32_t i) override;

protected:
  virtual bool loadSample(const char *name) override;
  bool Load(WavFile &wave);

private:
  static uint32_t writeOffset1_;
  static uint32_t writeOffset2_;
  static uint32_t storeLimit1_;
  static uint32_t storeLimit2_;
};

#endif // _DESKTOP_SAMPLE_POOL_H_
