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

#include "MidiOutDevice.h"

MidiOutDevice::MidiOutDevice(const char *name) { name_ = name; };

MidiOutDevice::~MidiOutDevice(){};

const char *MidiOutDevice::GetName() { return name_.c_str(); };

void MidiOutDevice::SetName(const char *name) { name_ = name; }

void MidiOutDevice::SendQueue(
    etl::vector<MidiMessage, MIDI_MAX_MESG_QUEUE> &queue) {
  for (auto &msg : queue) {
    SendMessage(msg);
  }
}

void MidiOutDevice::SendRawBytes(const uint8_t *data, uint8_t len) {
  (void)data;
  (void)len;
}
