/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _ADVUSBMIDIINDEVICE_H_
#define _ADVUSBMIDIINDEVICE_H_

#include "Services/Midi/MidiInDevice.h"

extern "C" void advUSBMidiHostFeedRxBytes(const uint8_t *bytes, uint16_t count);

class advUSBMidiInDevice : public MidiInDevice {
public:
  advUSBMidiInDevice(const char *name);
  virtual ~advUSBMidiInDevice();

  virtual bool Init();
  virtual void Close();
  virtual bool Start();
  virtual void Stop();

  virtual void poll();

protected:
  friend void advUSBMidiHostFeedRxBytes(const uint8_t *bytes, uint16_t count);

  virtual bool initDriver() { return true; };
  virtual bool startDriver() { return true; };
  virtual void stopDriver() {}
  virtual void closeDriver() {}
  MidiInputSource getInputSource() const override {
    return MidiInputSource::Usb;
  }
};

#endif
