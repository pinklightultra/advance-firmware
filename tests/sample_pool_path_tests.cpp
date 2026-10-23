/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Instruments/SamplePool.h"
#include "Application/Instruments/WavHeader.h"
#include "System/FileSystem/FakeFileSystem.h"
#include "doctest/doctest.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace {

class FileSystemInstallGuard {
public:
  explicit FileSystemInstallGuard(FileSystem *fs) { FileSystem::Install(fs); }
  ~FileSystemInstallGuard() { FileSystem::Install(nullptr); }
};

class TestSamplePool : public SamplePool {
public:
  void Reset() override {
    count_ = 0;
    loadedPaths_.clear();
    unloadRequests_.clear();
    for (uint32_t i = 0; i < MAX_SAMPLES; ++i) {
      nameStore_[i][0] = '\0';
    }
  }

  bool CheckSampleFits(int) override { return true; }
  uint32_t GetAvailableSampleStorageSpace() override { return 0; }

  bool unloadSample(uint32_t index) override {
    unloadRequests_.push_back(index);
    if (index >= count_) {
      return false;
    }

    if (count_ > 0) {
      --count_;
      nameStore_[count_][0] = '\0';
    }
    return true;
  }

  void SeedLoadedSample(const char *name) {
    count_ = 1;
    std::strncpy(nameStore_[0], name, MAX_INSTRUMENT_FILENAME_LENGTH);
    nameStore_[0][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
  }

  const std::vector<std::string> &loadedPaths() const { return loadedPaths_; }
  const std::vector<uint32_t> &unloadRequests() const {
    return unloadRequests_;
  }

  std::string storedName(uint32_t index) const {
    if (index >= count_) {
      return "";
    }
    return nameStore_[index];
  }

protected:
  bool loadSample(const char *name) override {
    loadedPaths_.emplace_back(name != nullptr ? name : "");
    if (count_ >= MAX_SAMPLES) {
      return false;
    }

    std::strncpy(nameStore_[count_], name != nullptr ? name : "",
                 MAX_INSTRUMENT_FILENAME_LENGTH);
    nameStore_[count_][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
    ++count_;
    return true;
  }

private:
  std::vector<std::string> loadedPaths_;
  std::vector<uint32_t> unloadRequests_;
};

void writeTestWav(FakeFileSystem &fs, const char *path, uint32_t sampleRate,
                  uint16_t channelCount = 1, const int16_t *samples = nullptr,
                  uint32_t frameCount = 4) {
  auto file = fs.Open(path, "w");
  REQUIRE(file);
  REQUIRE(
      WavHeaderWriter::WriteHeader(file.get(), sampleRate, channelCount, 16));

  static const int16_t defaultMonoSamples[] = {0, 1024, -1024, 0};
  const int16_t *sampleData = samples != nullptr ? samples : defaultMonoSamples;
  const uint32_t sampleCount = frameCount * channelCount;
  REQUIRE(file->Write(sampleData, sizeof(int16_t), sampleCount) ==
          static_cast<int>(sampleCount * sizeof(int16_t)));
  REQUIRE(
      WavHeaderWriter::UpdateFileSize(file.get(), frameCount, channelCount, 2));
}

std::string readFile(FakeFileSystem &fs, const char *path) {
  auto file = fs.Open(path, "r");
  REQUIRE(file);

  std::string contents;
  char buffer[64];
  while (true) {
    const int bytesRead = file->Read(buffer, sizeof(buffer));
    REQUIRE(bytesRead >= 0);
    if (bytesRead == 0) {
      break;
    }
    contents.append(buffer, static_cast<size_t>(bytesRead));
  }

  return contents;
}

std::vector<int16_t> readPcm16Data(FakeFileSystem &fs, const char *path,
                                   WavHeaderInfo &headerInfo) {
  auto file = fs.Open(path, "r");
  REQUIRE(file);

  auto header = WavHeaderWriter::ReadHeader(file.get());
  REQUIRE(header.has_value());
  headerInfo = header.value();

  file->Seek(headerInfo.dataOffset, SEEK_SET);
  const size_t sampleCount = headerInfo.dataChunkSize / sizeof(int16_t);
  std::vector<int16_t> samples(sampleCount);
  if (sampleCount > 0) {
    REQUIRE(file->Read(samples.data(),
                       static_cast<int>(sampleCount * sizeof(int16_t))) ==
            static_cast<int>(sampleCount * sizeof(int16_t)));
  }

  return samples;
}

bool operationLogHasNoChdir(const FakeFileSystem &fs) {
  return std::none_of(fs.operationLog().begin(), fs.operationLog().end(),
                      [](const FakeFileSystemOperation &entry) {
                        return entry.type == FakeFileSystemOperationType::Chdir;
                      });
}

} // namespace

TEST_CASE(
    "SamplePool imports from an absolute source path without cwd changes") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/imports");
  fs.AddDirectory("/projects/demo/samples");
  writeTestWav(fs, "/imports/Drum Loop.wav", 44100);

  TestSamplePool pool;
  fs.ClearOperationLog();

  const int sampleIndex =
      pool.ImportSample("/imports/Drum Loop.wav", "demo", 2);

  REQUIRE(sampleIndex == 0);
  REQUIRE(pool.loadedPaths().size() == 1);
  CHECK(pool.loadedPaths().front() == "/projects/demo/samples/Drum Loop.wav");
  CHECK(pool.storedName(0) == "Drum Loop.wav");
  CHECK(operationLogHasNoChdir(fs));
  CHECK(std::any_of(fs.operationLog().begin(), fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type == FakeFileSystemOperationType::Open &&
                             entry.path == "/imports/Drum Loop.wav" &&
                             entry.resolvedPath == "/imports/Drum Loop.wav" &&
                             entry.success;
                    }));
}

TEST_CASE("SamplePool import truncates overlong source leaf names to a valid "
          "project sample filename") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/imports");
  fs.AddDirectory("/projects/demo/samples");
  writeTestWav(fs, "/imports/abcdefghijklmnopqrstuvwxyz.wav", 44100);

  TestSamplePool pool;
  fs.ClearOperationLog();

  const int sampleIndex =
      pool.ImportSample("/imports/abcdefghijklmnopqrstuvwxyz.wav", "demo", 2);

  REQUIRE(sampleIndex == 0);
  REQUIRE(pool.loadedPaths().size() == 1);
  CHECK(pool.loadedPaths().front() ==
        "/projects/demo/samples/abcdefghijklmnopqrst.wav");
  CHECK(pool.storedName(0) == "abcdefghijklmnopqrst.wav");
  CHECK(operationLogHasNoChdir(fs));
  CHECK(std::any_of(fs.operationLog().begin(), fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type == FakeFileSystemOperationType::Open &&
                             entry.path == "/projects/demo/samples/"
                                           "abcdefghijklmnopqrst.wav" &&
                             entry.resolvedPath == "/projects/demo/samples/"
                                                   "abcdefghijklmnopqrst.wav" &&
                             entry.success;
                    }));
}

TEST_CASE("SamplePool reload preserves the leaf filename when given an "
          "absolute path") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);

  TestSamplePool pool;
  pool.SeedLoadedSample("Snare.wav");
  fs.ClearOperationLog();

  const int8_t sampleIndex =
      pool.ReloadSample(0, "/projects/demo/samples/Snare.wav");

  REQUIRE(sampleIndex == 0);
  REQUIRE(pool.loadedPaths().size() == 1);
  REQUIRE(pool.unloadRequests().size() == 1);
  CHECK(pool.unloadRequests().front() == 0);
  CHECK(pool.loadedPaths().front() == "/projects/demo/samples/Snare.wav");
  CHECK(pool.storedName(0) == "Snare.wav");
  CHECK(operationLogHasNoChdir(fs));
}

TEST_CASE("SamplePool loads project samples from an explicit absolute "
          "directory without cwd changes") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/projects/demo/samples");
  writeTestWav(fs, "/projects/demo/samples/Kick.wav", 44100);
  writeTestWav(fs, "/projects/demo/samples/Snare.wav", 44100);

  TestSamplePool pool;
  fs.ClearOperationLog();

  pool.Load("demo");

  REQUIRE(pool.loadedPaths().size() == 2);
  CHECK(std::all_of(pool.loadedPaths().begin(), pool.loadedPaths().end(),
                    [](const std::string &path) {
                      return path.rfind("/projects/demo/samples/", 0) == 0;
                    }));
  CHECK(std::find(pool.loadedPaths().begin(), pool.loadedPaths().end(),
                  "/projects/demo/samples/Kick.wav") !=
        pool.loadedPaths().end());
  CHECK(std::find(pool.loadedPaths().begin(), pool.loadedPaths().end(),
                  "/projects/demo/samples/Snare.wav") !=
        pool.loadedPaths().end());
  CHECK(operationLogHasNoChdir(fs));
  CHECK(std::any_of(fs.operationLog().begin(), fs.operationLog().end(),
                    [](const FakeFileSystemOperation &entry) {
                      return entry.type ==
                                 FakeFileSystemOperationType::ListDirectory &&
                             entry.resolvedPath == "/projects/demo/samples" &&
                             entry.success;
                    }));
}

TEST_CASE("SamplePool keeps exact 44.1 kHz imports bit-identical") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/imports");
  fs.AddDirectory("/projects/demo/samples");
  writeTestWav(fs, "/imports/Exact441.wav", 44100);

  const std::string sourceContents = readFile(fs, "/imports/Exact441.wav");

  TestSamplePool pool;
  const int sampleIndex = pool.ImportSample("/imports/Exact441.wav", "demo", 2);

  REQUIRE(sampleIndex == 0);
  CHECK(readFile(fs, "/projects/demo/samples/Exact441.wav") == sourceContents);
}

TEST_CASE("SamplePool resamples non-44.1 kHz imports to 44.1 kHz") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/imports");
  fs.AddDirectory("/projects/demo/samples");
  writeTestWav(fs, "/imports/ResampleMe.wav", 48000);

  TestSamplePool pool;
  const int sampleIndex =
      pool.ImportSample("/imports/ResampleMe.wav", "demo", 2);

  REQUIRE(sampleIndex == 0);

  auto imported = fs.Open("/projects/demo/samples/ResampleMe.wav", "r");
  REQUIRE(imported);

  auto header = WavHeaderWriter::ReadHeader(imported.get());
  REQUIRE(header.has_value());
  CHECK(header->sampleRate == 44100);
  CHECK(header->dataChunkSize > 0);
}

TEST_CASE("SamplePool downmixes stereo imports to mono for mono projects") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/imports");
  fs.AddDirectory("/projects/demo/samples");

  const std::array<int16_t, 8> stereoSamples = {1000,  -1000, 3000, 1000,
                                                -2000, -4000, 2000, 2000};
  writeTestWav(fs, "/imports/StereoToMono.wav", 44100, 2, stereoSamples.data(),
               4);

  TestSamplePool pool;
  const int sampleIndex =
      pool.ImportSample("/imports/StereoToMono.wav", "demo", 1);

  REQUIRE(sampleIndex == 0);
  WavHeaderInfo headerInfo;
  const std::vector<int16_t> imported =
      readPcm16Data(fs, "/projects/demo/samples/StereoToMono.wav", headerInfo);

  CHECK(headerInfo.sampleRate == 44100);
  CHECK(headerInfo.numChannels == 1);
  CHECK(imported == std::vector<int16_t>({0, 2000, -3000, 2000}));
}

TEST_CASE("SamplePool downmixes resampled stereo imports to mono for mono "
          "projects") {
  FakeFileSystem fs;
  FileSystemInstallGuard installGuard(&fs);
  fs.AddDirectory("/imports");
  fs.AddDirectory("/projects/demo/samples");

  const std::array<int16_t, 8> stereoSamples = {0,     1000,  1000, 0,
                                                -1000, -1000, 0,    0};
  writeTestWav(fs, "/imports/StereoResampleToMono.wav", 48000, 2,
               stereoSamples.data(), 4);

  TestSamplePool pool;
  const int sampleIndex =
      pool.ImportSample("/imports/StereoResampleToMono.wav", "demo", 1);

  REQUIRE(sampleIndex == 0);

  auto imported =
      fs.Open("/projects/demo/samples/StereoResampleToMono.wav", "r");
  REQUIRE(imported);
  auto header = WavHeaderWriter::ReadHeader(imported.get());
  REQUIRE(header.has_value());
  CHECK(header->sampleRate == 44100);
  CHECK(header->numChannels == 1);
  CHECK(header->dataChunkSize > 0);
}
