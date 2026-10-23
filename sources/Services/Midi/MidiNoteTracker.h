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

#ifndef _MIDI_NOTE_TRACKER_H_
#define _MIDI_NOTE_TRACKER_H_

#include "Externals/etl/include/etl/array.h"
#include "MidiInputSource.h"
#include <cstdint>

// Maximum number of channels to track notes on
#define MAX_NOTE_CHANNELS 8

/**
 * MidiNoteTracker - Tracks active MIDI notes across multiple channels
 *
 * This class keeps track of which notes are currently active on which channels,
 * allowing for polyphonic note handling where a note is only stopped when
 * all instances of it have been released.
 */
class MidiNoteTracker {
public:
  // The fixed-size note state is value-initialized with every slot inactive.
  MidiNoteTracker() = default;
  // Nothing needs to be cleaned up.
  ~MidiNoteTracker() = default;

  /**
   * Register a note as active on a specific channel
   *
   * @param inputSource The physical MIDI input that received the note
   * @param note The MIDI note number (0-127)
   * @param midiChannel The original MIDI channel (0-15)
   * @param instrumentChannel The instrument channel assigned (0-7)
   * @param velocity The note velocity (0-127)
   * @return True if the note was successfully registered
   */
  bool registerNote(MidiInputSource inputSource, uint8_t note,
                    uint8_t midiChannel, uint8_t instrumentChannel,
                    uint8_t velocity);

  /**
   * Check if a note is currently active on any channel
   *
   * @param note The MIDI note number (0-127)
   * @return True if the note is active on any channel
   */
  bool isNoteActive(uint8_t note) const;

  /**
   * Check if a specific note is active on a specific MIDI channel
   *
   * @param inputSource The physical MIDI input that received the note
   * @param note The MIDI note number (0-127)
   * @param midiChannel The MIDI channel (0-15)
   * @return True if the note is active on the specified channel
   */
  bool isNoteActiveOnChannel(MidiInputSource inputSource, uint8_t note,
                             uint8_t midiChannel) const;

  /**
   * Get the audio channel assigned to a specific note on a specific MIDI
   * channel
   *
   * @param inputSource The physical MIDI input that received the note
   * @param note The MIDI note number (0-127)
   * @param midiChannel The MIDI channel (0-15)
   * @return The audio channel assigned to the note, or -1 if not found
   */
  int getAudioChannelForNote(MidiInputSource inputSource, uint8_t note,
                             uint8_t midiChannel) const;

  /**
   * Unregister a note on a specific MIDI channel
   *
   * @param inputSource The physical MIDI input that received the note
   * @param note The MIDI note number (0-127)
   * @param midiChannel The MIDI channel (0-15)
   * @return The instrument channel that should be stopped, or -1 if the note
   * should not be stopped
   */
  int unregisterNote(MidiInputSource inputSource, uint8_t note,
                     uint8_t midiChannel);

  /**
   * Get the next available instrument channel
   *
   * @return The next available instrument channel, or -1 if all channels are in
   * use
   */
  int getNextAvailableChannel() const;

  /**
   * Get a mask of song channels currently occupied by MIDI notes
   *
   * @return A bit mask where bit 0 represents song channel 0
   */
  uint16_t getActiveChannelMask() const;

  /**
   * Clear every note owned by a physical MIDI input
   *
   * @param inputSource The physical MIDI input whose notes should be cleared
   * @return A bit mask of the song channels that were cleared
   */
  uint16_t clearInputSource(MidiInputSource inputSource);

  /**
   * Clear the note assigned to a song channel
   *
   * @param instrumentChannel The song channel to clear (0-7)
   */
  void clearChannel(uint8_t instrumentChannel);

  /**
   * Clear all tracked notes
   */
  void clear();

private:
  struct NoteInfo {
    bool active;
    MidiInputSource inputSource; // The physical input that owns the note
    uint8_t midiNote;            // The MIDI note number (0-127)
    uint8_t midiChannel;         // The original MIDI channel (0-15)
    uint8_t instrumentChannel;   // The instrument channel assigned (0-7)
    uint8_t velocity;            // The note velocity (0-127)
  };

  // Array of 8 playing notes, one for each instrument channel
  etl::array<NoteInfo, MAX_NOTE_CHANNELS> playingNotes_{};
};

#endif // _MIDI_NOTE_TRACKER_H_
