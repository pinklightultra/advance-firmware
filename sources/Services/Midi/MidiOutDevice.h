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

#ifndef _MIDIOUT_DEVICE_H_
#define _MIDIOUT_DEVICE_H_

#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/vector.h"
#include "Foundation/Types/Types.h"
#include "MidiMessage.h"
#include "config/StringLimits.h"

class MidiOutDevice {
public:
  MidiOutDevice(const char *name);
  virtual ~MidiOutDevice();

  const char *GetName();
  void SetName(const char *name);

  virtual bool Init() = 0;
  virtual void Close() = 0;
  virtual bool Start() = 0;
  virtual void Stop() = 0;

  /*! Sends a whole queue of messages - default implementation
          is to send every message one after the other using sendmessage
  */

  virtual void SendQueue(etl::vector<MidiMessage, MIDI_MAX_MESG_QUEUE> &queue);
  virtual void SendMessage(MidiMessage &m) = 0;
  virtual void SendRawBytes(const uint8_t *data, uint8_t len);

private:
  etl::string<STRING_MIDI_OUT_NAME_MAX> name_;
};
#endif
