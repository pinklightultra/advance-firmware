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

bool containsProjectParameterValue(const std::string &xml,
                                   const char *parameterName,
                                   const char *value) {
  const std::string nameNeedle = std::string("NAME=\"") + parameterName + "\"";
  const size_t namePos = xml.find(nameNeedle);
  if (namePos == std::string::npos) {
    return false;
  }

  const size_t parameterEnd = xml.find("/>", namePos);
  if (parameterEnd == std::string::npos) {
    return false;
  }

  const std::string valueNeedle = std::string("VALUE=\"") + value + "\"";
  const size_t valuePos = xml.find(valueNeedle, namePos);
  return valuePos != std::string::npos && valuePos < parameterEnd;
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

TEST_CASE("Project audio mode defaults to stereo when missing from persisted "
          "data") {
  BoundaryTestEnvironment env;
  env.fs.AddFile("/project-missing-audio-mode.xml",
                 "<PICOTRACKER><PROJECT VERSION=\"2.3-Beta3\">"
                 "<PARAMETER NAME=\"tempo\" VALUE=\"138\"/>"
                 "</PROJECT></PICOTRACKER>");

  Project project("Demo");
  project.Load("Demo");

  PersistencyDocument doc;
  REQUIRE(loadProjectNode(doc, "/project-missing-audio-mode.xml"));

  project.RestoreContent(&doc);

  Variable *audioMode = project.FindVariable(FourCC::VarProjectAudioMode);
  REQUIRE(audioMode != nullptr);
  CHECK(project.GetAudioMode() == kProjectAudioModeStereo);
  CHECK(audioMode->GetInt() == kProjectAudioModeStereo);
  CHECK(audioMode->GetString() == "stereo");
  CHECK(project.song_.GetChannelCount() == SONG_CHANNEL_COUNT_STEREO);
}

TEST_CASE("Project save persists the audio mode parameter") {
  Project stereoProject("Stereo");
  const std::string stereoXml = saveProjectContentToXml(stereoProject);
  CHECK(containsProjectParameterValue(stereoXml, "projectaudiomode", "stereo"));

  Project monoProject("Mono");
  Variable *audioMode = monoProject.FindVariable(FourCC::VarProjectAudioMode);
  REQUIRE(audioMode != nullptr);
  audioMode->SetInt(kProjectAudioModeMono, false);

  const std::string monoXml = saveProjectContentToXml(monoProject);
  CHECK(containsProjectParameterValue(monoXml, "projectaudiomode", "mono"));
}

TEST_CASE("Project restore syncs the song channel count from audio mode") {
  BoundaryTestEnvironment env;
  env.fs.AddFile("/project-mono-audio-mode.xml",
                 "<PICOTRACKER><PROJECT VERSION=\"2.3-Beta3\">\n"
                 "<PARAMETER NAME=\"projectaudiomode\" VALUE=\"mono\"/>"
                 "</PROJECT></PICOTRACKER>");

  Project project("Mono");
  project.Load("Mono");

  PersistencyDocument doc;
  REQUIRE(loadProjectNode(doc, "/project-mono-audio-mode.xml"));

  project.RestoreContent(&doc);

  Variable *audioMode = project.FindVariable(FourCC::VarProjectAudioMode);
  REQUIRE(audioMode != nullptr);
  CHECK(project.GetAudioMode() == kProjectAudioModeMono);
  CHECK(audioMode->GetInt() == kProjectAudioModeMono);
  CHECK(project.song_.GetChannelCount() == SONG_CHANNEL_COUNT_MONO);
}

TEST_CASE("Song save and restore preserves stereo row packing") {
  Song song;
  song.Reset();
  song.SetChannelCount(SONG_CHANNEL_COUNT_STEREO);
  song.data_[0] = 0x11;
  song.data_[SONG_CHANNEL_COUNT_STEREO - 1] = 0x22;
  song.data_[SONG_CHANNEL_COUNT_STEREO] = 0x33;

  const std::string xml = saveSongContentToXml(song);

  BoundaryTestEnvironment env;
  env.fs.AddFile("/song-stereo.xml", xml.c_str());

  PersistencyDocument doc;
  REQUIRE(loadSongNode(doc, "/song-stereo.xml"));

  Song restored;
  restored.Reset();
  restored.SetChannelCount(SONG_CHANNEL_COUNT_STEREO);
  restored.RestoreContent(&doc);

  CHECK(restored.data_[0] == 0x11);
  CHECK(restored.data_[SONG_CHANNEL_COUNT_STEREO - 1] == 0x22);
  CHECK(restored.data_[SONG_CHANNEL_COUNT_STEREO] == 0x33);
}

TEST_CASE("Song save and restore preserves mono-only channel columns") {
  Song song;
  song.Reset();
  song.SetChannelCount(SONG_CHANNEL_COUNT_MONO);
  song.data_[SONG_CHANNEL_COUNT_MONO - 1] = 0x44;
  song.data_[SONG_CHANNEL_COUNT_MONO] = 0x55;

  const std::string xml = saveSongContentToXml(song);

  BoundaryTestEnvironment env;
  env.fs.AddFile("/song-mono.xml", xml.c_str());

  PersistencyDocument doc;
  REQUIRE(loadSongNode(doc, "/song-mono.xml"));

  Song restored;
  restored.Reset();
  restored.SetChannelCount(SONG_CHANNEL_COUNT_MONO);
  restored.RestoreContent(&doc);

  CHECK(restored.data_[SONG_CHANNEL_COUNT_MONO - 1] == 0x44);
  CHECK(restored.data_[SONG_CHANNEL_COUNT_MONO] == 0x55);
}
