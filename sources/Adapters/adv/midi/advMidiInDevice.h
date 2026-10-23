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

#ifndef _ADV_MIDI_IN_DEVICE_H_
#define _ADV_MIDI_IN_DEVICE_H_

#include "Services/Midi/MidiInDevice.h"

class advMidiInDevice : public MidiInDevice {
public:
  advMidiInDevice(const char *name);
  virtual ~advMidiInDevice();

  // Poll the MIDI input buffer for new messages
  virtual void poll();

  virtual bool Start();
  virtual void Stop();

protected:
  // Driver specific implementation
  virtual bool initDriver();
  virtual void closeDriver();
  virtual bool startDriver();
  virtual void stopDriver();
  MidiInputSource getInputSource() const override {
    return MidiInputSource::Trs;
  }
};
#endif
