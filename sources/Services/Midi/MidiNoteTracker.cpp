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

#include "MidiNoteTracker.h"
#include "System/Console/Trace.h"

bool MidiNoteTracker::registerNote(MidiInputSource inputSource, uint8_t note,
                                   uint8_t midiChannel,
                                   uint8_t instrumentChannel,
                                   uint8_t velocity) {
  // Validate parameters
  if (note > 127 || midiChannel > 15 ||
      instrumentChannel >= playingNotes_.size()) {
    Trace::Debug("Invalid parameters in registerNote: note=%d, midiChannel=%d, "
                 "instrumentChannel=%d",
                 note, midiChannel, instrumentChannel);
    return false;
  }

  // The caller selects an available reserved song channel for this note.
  if (playingNotes_[instrumentChannel].active) {
    return false;
  }

  // Register the note in the selected channel
  playingNotes_[instrumentChannel].active = true;
  playingNotes_[instrumentChannel].inputSource = inputSource;
  playingNotes_[instrumentChannel].midiNote = note;
  playingNotes_[instrumentChannel].midiChannel = midiChannel;
  playingNotes_[instrumentChannel].instrumentChannel = instrumentChannel;
  playingNotes_[instrumentChannel].velocity = velocity;

  Trace::Debug("Note %d registered on MIDI channel %d, audio channel %d", note,
               midiChannel, instrumentChannel);
  return true;
}

bool MidiNoteTracker::isNoteActive(uint8_t note) const {
  // Check if the note is active on any channel
  for (const auto &activeNote : playingNotes_) {
    if (activeNote.active && activeNote.midiNote == note) {
      return true;
    }
  }
  return false;
}

bool MidiNoteTracker::isNoteActiveOnChannel(MidiInputSource inputSource,
                                            uint8_t note,
                                            uint8_t midiChannel) const {
  // Check if the specific note is active on the specific MIDI channel
  for (const auto &activeNote : playingNotes_) {
    if (activeNote.active && activeNote.inputSource == inputSource &&
        activeNote.midiNote == note && activeNote.midiChannel == midiChannel) {
      return true;
    }
  }
  return false;
}

int MidiNoteTracker::getAudioChannelForNote(MidiInputSource inputSource,
                                            uint8_t note,
                                            uint8_t midiChannel) const {
  // Find the audio channel assigned to this note on this MIDI channel
  for (size_t i = 0; i < playingNotes_.size(); i++) {
    const auto &activeNote = playingNotes_[i];
    if (activeNote.active && activeNote.inputSource == inputSource &&
        activeNote.midiNote == note && activeNote.midiChannel == midiChannel) {
      // The audio channel is the index in the array
      return static_cast<int>(i);
    }
  }
  return -1; // Note not found
}

int MidiNoteTracker::getNextAvailableChannel() const {
  // Find an inactive channel
  for (size_t i = 0; i < playingNotes_.size(); i++) {
    if (!playingNotes_[i].active) {
      return static_cast<int>(i);
    }
  }

  // If all channels are active, return -1 (no available channels)
  return -1;
}

int MidiNoteTracker::unregisterNote(MidiInputSource inputSource, uint8_t note,
                                    uint8_t midiChannel) {
  // Find the note in the active notes list
  for (size_t i = 0; i < playingNotes_.size(); i++) {
    auto &activeNote = playingNotes_[i];
    if (activeNote.active && activeNote.inputSource == inputSource &&
        activeNote.midiNote == note && activeNote.midiChannel == midiChannel) {
      // Found the note, mark it as inactive
      int audioChannel = static_cast<int>(i); // Audio channel = array index
      activeNote.active = false;
      Trace::Debug("Note %d unregistered from MIDI channel %d, stopping "
                   "audio channel %d",
                   note, midiChannel, audioChannel);
      return audioChannel;
    }
  }

  // Note not found
  Trace::Debug("Note %d not found on MIDI channel %d", note, midiChannel);
  return -1;
}

uint16_t MidiNoteTracker::getActiveChannelMask() const {
  uint16_t mask = 0;
  for (size_t i = 0; i < playingNotes_.size(); i++) {
    if (playingNotes_[i].active) {
      mask |= static_cast<uint16_t>(1U << static_cast<uint16_t>(i));
    }
  }
  return mask;
}

uint16_t MidiNoteTracker::clearInputSource(MidiInputSource inputSource) {
  uint16_t clearedChannelMask = 0;
  for (size_t i = 0; i < playingNotes_.size(); i++) {
    auto &activeNote = playingNotes_[i];
    if (activeNote.active && activeNote.inputSource == inputSource) {
      activeNote.active = false;
      clearedChannelMask |=
          static_cast<uint16_t>(1U << static_cast<uint16_t>(i));
    }
  }
  return clearedChannelMask;
}

void MidiNoteTracker::clearChannel(uint8_t instrumentChannel) {
  if (instrumentChannel < playingNotes_.size()) {
    playingNotes_[instrumentChannel].active = false;
  }
}

void MidiNoteTracker::clear() {
  // Mark all notes as inactive
  for (auto &note : playingNotes_) {
    note.active = false;
  }
}
