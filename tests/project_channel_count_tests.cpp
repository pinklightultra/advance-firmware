/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Application/Model/Project.h"
#include "Application/Persistency/PersistencyDocument.h"
#include "boundary_test_utils.h"
#include "doctest/doctest.h"

#include "Externals/TinyXML2/tinyxml2.h"
#include <cstdio>
#include <string>

namespace {
bool loadProjectNode(PersistencyDocument &doc, const char *path) {
  if (!doc.Load(path)) {
    return false;
  }
  if (!doc.FirstChild() || strcmp(doc.ElemName(), "PICOTRACKER")) {
    return false;
  }
  if (!doc.FirstChild() || strcmp(doc.ElemName(), "PROJECT")) {
    return false;
  }
  return true;
}

bool loadSongNode(PersistencyDocument &doc, const char *path) {
  if (!doc.Load(path)) {
    return false;
  }
  return doc.FirstChild() && strcmp(doc.ElemName(), "SONG") == 0;
}

class MemoryFile : public I_File {
public:
  MemoryFile() : cursor_(0), error_(0), closed_(false) {}

  int Read(void *ptr, int size) override {
    if (closed_ || ptr == nullptr || size < 0) {
      error_ = 1;
      return -1;
    }

    const size_t remaining =
        cursor_ < contents_.size() ? (contents_.size() - cursor_) : 0;
    const size_t bytesToRead = std::min(remaining, static_cast<size_t>(size));
    if (bytesToRead == 0) {
      return 0;
    }

    std::memcpy(ptr, contents_.data() + cursor_, bytesToRead);
    cursor_ += bytesToRead;
    return static_cast<int>(bytesToRead);
  }

  int GetC() override {
    unsigned char ch = 0;
    const int read = Read(&ch, 1);
    if (read != 1) {
      return EOF;
    }
    return ch;
  }

  int Write(const void *ptr, int size, int nmemb) override {
    if (closed_ || ptr == nullptr || size < 0 || nmemb < 0) {
      error_ = 1;
      return -1;
    }

    const size_t bytesToWrite =
        static_cast<size_t>(size) * static_cast<size_t>(nmemb);
    if (cursor_ > contents_.size()) {
      contents_.resize(cursor_, '\0');
    }
    if ((cursor_ + bytesToWrite) > contents_.size()) {
      contents_.resize(cursor_ + bytesToWrite, '\0');
    }

    std::memcpy(contents_.data() + cursor_, ptr, bytesToWrite);
    cursor_ += bytesToWrite;
    return static_cast<int>(bytesToWrite);
  }

  void Seek(long offset, int whence) override {
    size_t target = 0;
    switch (whence) {
    case SEEK_SET:
      if (offset < 0) {
        error_ = 1;
        return;
      }
      target = static_cast<size_t>(offset);
      break;
    case SEEK_CUR:
      if (offset < 0 && static_cast<size_t>(-offset) > cursor_) {
        error_ = 1;
        return;
      }
      target = offset < 0 ? cursor_ - static_cast<size_t>(-offset)
                          : cursor_ + static_cast<size_t>(offset);
      break;
    case SEEK_END:
      if (offset < 0 && static_cast<size_t>(-offset) > contents_.size()) {
        error_ = 1;
        return;
      }
      target = offset < 0 ? contents_.size() - static_cast<size_t>(-offset)
                          : contents_.size() + static_cast<size_t>(offset);
      break;
    default:
      error_ = 1;
      return;
    }

    cursor_ = target;
  }

  long Tell() override { return static_cast<long>(cursor_); }

  int Error() override { return error_; }

  bool Sync() override { return !closed_; }

  void Dispose() override {}

  const std::string &contents() const { return contents_; }

protected:
  bool Close() override {
    closed_ = true;
    return true;
  }

private:
  std::string contents_;
  size_t cursor_;
  int error_;
  bool closed_;
};

std::string saveProjectContentToXml(Project &project) {
  MemoryFile file;
  tinyxml2::XMLPrinter printer(&file);
  printer.OpenElement("PROJECT");
  project.SaveContent(&printer);
  printer.CloseElement();
  return file.contents();
}

std::string saveSongContentToXml(Song &song) {
  MemoryFile file;
  tinyxml2::XMLPrinter printer(&file);
  printer.OpenElement("SONG");
  song.SaveContent(&printer);
  printer.CloseElement();
  return file.contents();
}
} // namespace

TEST_CASE("Project ignores the obsolete audio mode parameter") {
  BoundaryTestEnvironment env;
  env.fs.AddFile("/project-obsolete-audio-mode.xml",
                 "<PICOTRACKER><PROJECT VERSION=\"2.3-Beta3\">"
                 "<PARAMETER NAME=\"projectaudiomode\" VALUE=\"mono\"/>"
                 "</PROJECT></PICOTRACKER>");

  Project project("Demo");
  project.Load("Demo");

  PersistencyDocument doc;
  REQUIRE(loadProjectNode(doc, "/project-obsolete-audio-mode.xml"));

  project.RestoreContent(&doc);

  CHECK(SONG_CHANNEL_COUNT == 8);
  CHECK(project.FindVariable("projectaudiomode") == nullptr);
}

TEST_CASE("Project save omits the obsolete audio mode parameter") {
  Project project("Demo");
  const std::string xml = saveProjectContentToXml(project);
  CHECK(xml.find("projectaudiomode") == std::string::npos);
}

TEST_CASE("Song save and restore preserves eight-channel row packing") {
  Song song;
  song.Reset();
  song.data_[0] = 0x11;
  song.data_[SONG_CHANNEL_COUNT - 1] = 0x22;
  song.data_[SONG_CHANNEL_COUNT] = 0x33;
  song.SetMidiInputChannel(0, 1);
  song.SetMidiInputChannel(SONG_CHANNEL_COUNT - 1, 16);
  song.SetMidiDefaultProgram(0, MIDI_INPUT_MIN_PROGRAM);
  song.SetMidiDefaultProgram(MIDI_INPUT_CHANNEL_COUNT - 1,
                             MIDI_INPUT_MAX_PROGRAM);

  const std::string xml = saveSongContentToXml(song);

  BoundaryTestEnvironment env;
  env.fs.AddFile("/song-eight-channel.xml", xml.c_str());

  PersistencyDocument doc;
  REQUIRE(loadSongNode(doc, "/song-eight-channel.xml"));

  Song restored;
  restored.Reset();
  restored.RestoreContent(&doc);

  CHECK(restored.data_[0] == 0x11);
  CHECK(restored.data_[SONG_CHANNEL_COUNT - 1] == 0x22);
  CHECK(restored.data_[SONG_CHANNEL_COUNT] == 0x33);
  CHECK(restored.GetMidiInputChannel(0) == 1);
  CHECK(restored.GetMidiInputChannel(SONG_CHANNEL_COUNT - 1) == 16);
  CHECK(restored.GetMidiDefaultProgram(0) == MIDI_INPUT_MIN_PROGRAM);
  CHECK(restored.GetMidiDefaultProgram(MIDI_INPUT_CHANNEL_COUNT - 1) ==
        MIDI_INPUT_MAX_PROGRAM);
}

TEST_CASE("Song MIDI input assignments provide a reserved polyphony pool") {
  Song song;
  song.Reset();
  song.SetMidiInputChannel(1, 1);
  song.SetMidiInputChannel(3, 2);
  song.SetMidiInputChannel(6, 1);

  CHECK(song.FindAvailableMidiInputSongChannel(0, 0) == 1);
  CHECK(song.FindAvailableMidiInputSongChannel(0, 1U << 1) == 6);
  CHECK(song.FindAvailableMidiInputSongChannel(0, (1U << 1) | (1U << 6)) == -1);
  CHECK(song.FindAvailableMidiInputSongChannel(1, 0) == 3);
  CHECK(song.FindAvailableMidiInputSongChannel(2, 0) == -1);
}

TEST_CASE("Song rejects invalid MIDI input assignments") {
  Song song;
  song.Reset();

  song.SetMidiInputChannel(2, MIDI_INPUT_CHANNEL_COUNT + 1);

  CHECK_FALSE(song.IsMidiInputChannelAssigned(2));
}

TEST_CASE("Song MIDI default programs reset and reject invalid programs") {
  Song song;
  song.Reset();

  for (int midiChannel = 0; midiChannel < MIDI_INPUT_CHANNEL_COUNT;
       midiChannel++) {
    CHECK(song.GetMidiDefaultProgram(midiChannel) ==
          MIDI_INPUT_DEFAULT_PROGRAM);
    CHECK(song.GetMidiDefaultInstrumentIndex(midiChannel) == 0);
  }

  song.SetMidiDefaultProgram(4, 0x2A);
  CHECK(song.GetMidiDefaultProgram(4) == 0x2A);
  CHECK(song.GetMidiDefaultInstrumentIndex(4) == 0x2A);

  song.SetMidiDefaultProgram(4, MIDI_INPUT_MAX_PROGRAM);
  CHECK(song.GetMidiDefaultInstrumentIndex(4) == MAX_INSTRUMENT_COUNT - 1);

  song.SetMidiDefaultProgram(4, MIDI_INPUT_MIN_PROGRAM - 1);
  CHECK(song.GetMidiDefaultProgram(4) == MIDI_INPUT_DEFAULT_PROGRAM);

  song.SetMidiDefaultProgram(4, MIDI_INPUT_MAX_PROGRAM + 1);
  CHECK(song.GetMidiDefaultProgram(4) == MIDI_INPUT_DEFAULT_PROGRAM);
}
