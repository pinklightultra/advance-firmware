/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _MIDI_INPUT_SOURCE_H_
#define _MIDI_INPUT_SOURCE_H_

#include <cstdint>

enum class MidiInputSource : uint8_t {
  Trs,
  Usb,
};

#endif // _MIDI_INPUT_SOURCE_H_
