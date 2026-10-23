/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#pragma once

#include <stdint.h>

class MidiInDevice;
class MidiService;
struct MidiMessage;
class PlayerEvent;

class IControlSurface {
public:
  virtual ~IControlSurface() = default;

  virtual bool Init(MidiService &midiService) = 0;
  virtual bool Start() = 0;
  virtual void Stop() = 0;
  virtual void SetEnabled(bool enabled) = 0;

  virtual void RegisterMidiInput(MidiInDevice &device) = 0;

  virtual bool OnMidiMessage(MidiMessage &message) = 0;
  virtual void OnPlayerEvent(PlayerEvent &event) = 0;
  virtual void RefreshLeds() = 0;
  virtual bool HasSongWindow() const = 0;
  virtual uint32_t GetSongWindowOffset() const = 0;
  virtual uint32_t GetSongWindowRowCount() const = 0;
};
