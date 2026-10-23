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

#include "advMidiService.h"
#include "advMidiInDevice.h"
#include "advMidiOutDevice.h"
#include "advUSBMidiInDevice.h"
#include "advUSBMidiOutDevice.h"

advMidiService::advMidiService()
    : // Initialize static member variables with their respective names
      midiOutDevice_("MIDI OUT"), usbMidiOutDevice_("USB"),
      midiInDevice_("MIDI IN"), usbMidiInDevice_("USB MIDI IN") {
  // Add MIDI output devices to the output device list
  outList_.insert(outList_.end(), &midiOutDevice_);
  outList_.insert(outList_.end(), &usbMidiOutDevice_);
  registerUsbControlSurfaceOutDevice(usbMidiOutDevice_);

  // Add MIDI input devices to the input device list
  inList_.insert(inList_.end(), &midiInDevice_);
  inList_.insert(inList_.end(), &usbMidiInDevice_);
};

advMidiService::~advMidiService(){};

void advMidiService::poll() {
  // Poll all MIDI input devices
  for (auto dev : inList_) {
    if (dev != nullptr) {
      dev->poll();
    }
  }
};

void advMidiService::registerControlSurfaceInputs(IControlSurface &surface) {
  surface.RegisterMidiInput(usbMidiInDevice_);
}
