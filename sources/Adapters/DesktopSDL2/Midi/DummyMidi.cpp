/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * DummyMidi.cpp - Desktop MIDI Service for picoTracker Advance
 */

#include "DummyMidi.h"

DummyMidi::DummyMidi()
    : dummyIn1_("MIDI IN 1", MidiInputSource::Trs),
      dummyIn2_("MIDI IN 2", MidiInputSource::Usb),
      dummyOut1_("MIDI OUT 1"), dummyOut2_("MIDI OUT 2") {
  outList_.insert(outList_.end(), &dummyOut1_);
  outList_.insert(outList_.end(), &dummyOut2_);
  inList_.insert(inList_.end(), &dummyIn1_);
  inList_.insert(inList_.end(), &dummyIn2_);
}

DummyMidi::~DummyMidi() {}
