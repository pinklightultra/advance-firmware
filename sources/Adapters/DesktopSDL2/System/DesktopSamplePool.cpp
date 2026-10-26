/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * DesktopSamplePool.cpp - SamplePool for picoTracker Advance on Desktop
 */

#include "DesktopSamplePool.h"
#include "Adapters/adv/system/SamplePoolStorage.h"
#include "System/Console/Trace.h"
#include <cstring>
#include <utility>
#include <vector>

static uint8_t sampleStore1[DESKTOP_STORE1_SIZE];
static uint8_t sampleStore2[DESKTOP_STORE2_SIZE];

uint32_t DesktopSamplePool::writeOffset1_ = 0;
uint32_t DesktopSamplePool::writeOffset2_ = 0;
uint32_t DesktopSamplePool::storeLimit1_ = DESKTOP_STORE1_SIZE;
uint32_t DesktopSamplePool::storeLimit2_ = DESKTOP_STORE2_SIZE;

DesktopSamplePool::DesktopSamplePool() : SamplePool() {}

DesktopSamplePool::~DesktopSamplePool() { Reset(); }

void DesktopSamplePool::Reset() {
  count_ = 0;
  for (int i = 0; i < MAX_SAMPLES; i++) {
    wav_[i].Close();
    nameStore_[i][0] = '\0';
  }
  writeOffset1_ = 0;
  writeOffset2_ = 0;
}

bool DesktopSamplePool::CheckSampleFits(uint32_t sampleSize) {
  return AdvanceSampleStorage::FitsInEither(
      writeOffset1_, storeLimit1_, writeOffset2_, storeLimit2_, sampleSize);
}

uint32_t DesktopSamplePool::GetAvailableSampleStorageSpace() {
  return AdvanceSampleStorage::GetAvailableCapacity(writeOffset1_, storeLimit1_) +
         AdvanceSampleStorage::GetAvailableCapacity(writeOffset2_, storeLimit2_);
}

bool DesktopSamplePool::loadSample(const char *name) {
  Trace::Log("SAMPLEPOOL", "Loading sample into RAM: %s", name);

  if (count_ == MAX_SAMPLES)
    return false;

  const uint32_t slot = count_;
  auto res = wav_[slot].Open(name);
  if (!res) {
    Trace::Error("Failed to load sample: %s", name);
    return false;
  }

  if (!Load(wav_[slot])) {
    wav_[slot].Close();
    return false;
  }

  wav_[slot].Close();
  strncpy(nameStore_[slot], name, MAX_INSTRUMENT_FILENAME_LENGTH);
  nameStore_[slot][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
  count_++;

  return true;
}

bool DesktopSamplePool::Load(WavFile &wave) {
  uint32_t fileSize = wave.GetDiskSize(-1);
  Trace::Debug("File size: %u", fileSize);

  uint32_t *writeOffset = nullptr;
  uint8_t *sampleStore = nullptr;
  const uint32_t available1 =
      AdvanceSampleStorage::GetAvailableCapacity(writeOffset1_, storeLimit1_);
  const uint32_t available2 =
      AdvanceSampleStorage::GetAvailableCapacity(writeOffset2_, storeLimit2_);

  if (fileSize <= available1 && fileSize <= available2) {
    if (available1 < available2) {
      writeOffset = &writeOffset1_;
      sampleStore = sampleStore1;
    } else {
      writeOffset = &writeOffset2_;
      sampleStore = sampleStore2;
    }
  } else if (fileSize <= available1) {
    writeOffset = &writeOffset1_;
    sampleStore = sampleStore1;
  } else if (fileSize <= available2) {
    writeOffset = &writeOffset2_;
    sampleStore = sampleStore2;
  } else {
    Trace::Error("Sample does not fit into RAM storage");
    return false;
  }

  *writeOffset += AdvanceSampleStorage::GetAlignmentPadding(*writeOffset);
  wave.SetSampleBuffer((short *)(sampleStore + *writeOffset));

  uint32_t br = 0;
  uint32_t writeStep = (9 + fileSize) / 10;
  uint32_t totalWritten = 0;
  uint32_t subStep = 0;

  wave.Rewind();
  wave.Read(sampleStore + *writeOffset, BUFFER_SIZE, &br);

  while (br > 0) {
    totalWritten += br;
    while (totalWritten >= writeStep) {
      totalWritten -= writeStep;
      subStep++;
      updateStatus(importIndex * 10 + subStep, importCount * 10, "Importing");
    }
    *writeOffset += br;
    wave.Read(sampleStore + *writeOffset, BUFFER_SIZE, &br);
  }

  return true;
}

bool DesktopSamplePool::unloadSample(uint32_t index) {
  if (index >= count_) {
    return false;
  }

  uint8_t *moveDst = static_cast<uint8_t *>(wav_[index].GetSampleBuffer(0));
  if (!moveDst) {
    return false;
  }

  uint8_t *poolBase = nullptr;
  uint32_t poolLimit = 0;
  uint32_t *poolWriteOffset = nullptr;

  if (moveDst >= sampleStore1 && moveDst < sampleStore1 + storeLimit1_) {
    poolBase = sampleStore1;
    poolLimit = storeLimit1_;
    poolWriteOffset = &writeOffset1_;
  } else if (moveDst >= sampleStore2 && moveDst < sampleStore2 + storeLimit2_) {
    poolBase = sampleStore2;
    poolLimit = storeLimit2_;
    poolWriteOffset = &writeOffset2_;
  } else {
    return false;
  }

  const uint32_t moveDstOffset = static_cast<uint32_t>(moveDst - poolBase);
  if (moveDstOffset >= *poolWriteOffset) {
    return false;
  }

  uint8_t *moveSrc = nullptr;
  for (uint32_t j = 0; j < count_; ++j) {
    if (j == index) continue;
    auto *candidate = static_cast<uint8_t *>(wav_[j].GetSampleBuffer(0));
    if (candidate && candidate >= poolBase && candidate < poolBase + poolLimit &&
        candidate > moveDst) {
      if (moveSrc == nullptr || candidate < moveSrc) {
        moveSrc = candidate;
      }
    }
  }

  uint32_t shift = 0;
  uint32_t bytesToMove = 0;
  if (moveSrc != nullptr) {
    shift = static_cast<uint32_t>(moveSrc - moveDst);
    bytesToMove = *poolWriteOffset - static_cast<uint32_t>(moveSrc - poolBase);
  } else {
    shift = *poolWriteOffset - moveDstOffset;
  }

  if (bytesToMove > 0) {
    std::memmove(moveDst, moveSrc, bytesToMove);
  }

  *poolWriteOffset -= shift;

  if (shift > 0 && moveSrc != nullptr) {
    for (uint32_t j = 0; j < count_; ++j) {
      if (j == index) continue;
      auto *buf = static_cast<uint8_t *>(wav_[j].GetSampleBuffer(0));
      if (buf && buf >= poolBase && buf < poolBase + poolLimit && buf >= moveSrc) {
        wav_[j].SetSampleBuffer(reinterpret_cast<short *>(buf - shift));
      }
    }
  }

  for (uint32_t j = index; j < count_ - 1; ++j) {
    wav_[j] = std::move(wav_[j + 1]);
    memcpy(nameStore_[j], nameStore_[j + 1], MAX_INSTRUMENT_FILENAME_LENGTH + 1);
  }

  wav_[count_ - 1].Close();
  nameStore_[count_ - 1][0] = '\0';
  count_--;

  return true;
}
