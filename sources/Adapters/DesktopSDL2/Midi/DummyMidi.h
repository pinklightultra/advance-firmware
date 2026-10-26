/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * DummyMidi.h - Desktop MIDI Service for picoTracker Advance
 */

#ifndef _DESKTOP_DUMMY_MIDI_H_
#define _DESKTOP_DUMMY_MIDI_H_

#include "Services/Midi/MidiService.h"
#include "Services/Midi/MidiInDevice.h"
#include "Services/Midi/MidiOutDevice.h"

class DummyMidiInDevice : public MidiInDevice {
public:
  DummyMidiInDevice(const char *name, MidiInputSource src) : MidiInDevice(name), source_(src) {}
  virtual ~DummyMidiInDevice() {}
  virtual bool Start() override { return true; }
  virtual void Stop() override {}
  virtual void poll() override {}

protected:
  virtual bool initDriver() override { return true; }
  virtual void closeDriver() override {}
  virtual bool startDriver() override { return true; }
  virtual void stopDriver() override {}
  virtual MidiInputSource getInputSource() const override { return source_; }

private:
  MidiInputSource source_;
};

class DummyMidiOutDevice : public MidiOutDevice {
public:
  DummyMidiOutDevice(const char *name) : MidiOutDevice(name) {}
  virtual ~DummyMidiOutDevice() {}
  virtual bool Init() override { return true; }
  virtual void Close() override {}
  virtual bool Start() override { return true; }
  virtual void Stop() override {}
  virtual void SendMessage(MidiMessage &m) override {}
};

class DummyMidi : public MidiService {
public:
  DummyMidi();
  virtual ~DummyMidi();

private:
  DummyMidiInDevice dummyIn1_;
  DummyMidiInDevice dummyIn2_;
  DummyMidiOutDevice dummyOut1_;
  DummyMidiOutDevice dummyOut2_;
};

#endif // _DESKTOP_DUMMY_MIDI_H_
