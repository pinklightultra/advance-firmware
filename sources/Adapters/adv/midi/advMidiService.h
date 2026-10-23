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
#ifndef _ADVMIDISERVICE_H_
#define _ADVMIDISERVICE_H_

#include "Services/Midi/MidiService.h"
#include "advMidiInDevice.h"
#include "advMidiOutDevice.h"
#include "advUSBMidiInDevice.h"
#include "advUSBMidiOutDevice.h"

class advMidiService : public MidiService {
public:
  advMidiService();
  ~advMidiService();

  // Poll MIDI input devices for new messages
  void poll();

protected:
  void registerControlSurfaceInputs(IControlSurface &surface) override;

private:
  advMidiOutDevice midiOutDevice_;
  advUSBMidiOutDevice usbMidiOutDevice_;
  advMidiInDevice midiInDevice_;
  advUSBMidiInDevice usbMidiInDevice_;
};

#endif
