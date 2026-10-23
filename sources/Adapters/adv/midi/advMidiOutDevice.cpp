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

#include "advMidiOutDevice.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "usart.h"

advMidiOutDevice::advMidiOutDevice(const char *name) : MidiOutDevice(name) {}

bool advMidiOutDevice::Init() { return true; }

void advMidiOutDevice::Close(){};

bool advMidiOutDevice::Start() { return true; };

void advMidiOutDevice::Stop() {}

void advMidiOutDevice::SendImmediate(MidiMessage &msg) { SendMessage(msg); }

void advMidiOutDevice::SendMessage(MidiMessage &msg) {
  uint8_t status = msg.status_;
  HAL_UART_Transmit(&MIDI_UART, &status, 1, 1);

  if (msg.status_ < 0xF0) {
    uint8_t data1 = msg.data1_;
    HAL_UART_Transmit(&MIDI_UART, &data1, 1, 1);
    if (msg.data2_ != MidiMessage::UNUSED_BYTE) {
      uint8_t data2 = msg.data2_;
      HAL_UART_Transmit(&MIDI_UART, &data2, 1, 1);
    }
  }
}

void advMidiOutDevice::SendRawBytes(const uint8_t *data, uint8_t len) {
  if ((data == nullptr) || (len == 0)) {
    return;
  }
  HAL_UART_Transmit(&MIDI_UART, const_cast<uint8_t *>(data), len, 1);
}
