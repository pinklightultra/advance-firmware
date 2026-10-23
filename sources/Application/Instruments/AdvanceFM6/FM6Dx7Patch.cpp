/*
 * SPDX-License-Identifier: Apache-2.0 AND GPL-3.0-only
 *
 * Copyright 2012 Google Inc.
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Contains code adapted from libmsfa/msfa under Apache-2.0.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Original notices: sources/Application/Instruments/AdvanceFM6/NOTICE.
 * License text: LICENSES/Apache-2.0.txt at the repository root.
 * Existing license grants are preserved. See LICENSING.md.
 */

#include "FM6Dx7Patch.h"
#include "FM6PresetPatches.h"

#include "System/Console/nanoprintf.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace fm6 {
namespace {

void setRatioFromDx7(uint8_t coarse, uint8_t fine, uint8_t &ratioNumerator,
                     uint8_t &ratioDenominator) {
  double ratio = coarse == 0U ? 0.5 : static_cast<double>(coarse);
  ratio *= 1.0 + (static_cast<double>(fine) / 100.0);

  uint8_t denominator =
      ratio < 1.0 ? 255U : static_cast<uint8_t>(std::floor(255.0 / ratio));
  if (denominator == 0U) {
    denominator = 1U;
  }

  uint32_t numerator = static_cast<uint32_t>(
      std::lround(ratio * static_cast<double>(denominator)));
  numerator = std::clamp<uint32_t>(numerator, 1U, 255U);

  ratioNumerator = static_cast<uint8_t>(numerator);
  ratioDenominator = denominator;
}

} // namespace

uint32_t Dx7FixedFrequencyMilliHertz(uint8_t coarse, uint8_t fine) {
  const uint32_t exponentHundredths =
      static_cast<uint32_t>(coarse & 0x03U) * 100U +
      std::min<uint8_t>(fine, 99U);
  const double frequencyHz =
      std::pow(10.0, static_cast<double>(exponentHundredths) / 100.0);
  return static_cast<uint32_t>(std::lround(frequencyHz * 1000.0));
}

bool CheckDx7BankHeader(const uint8_t *header) {
  return header[0] == 0xF0U && header[1] == 0x43U &&
         (header[2] & 0xF0U) == 0U && header[3] == 0x09U &&
         header[4] == 0x20U && header[5] == 0x00U;
}

void ExtractDx7VoiceName(const uint8_t *voiceData, size_t patchIndex,
                         Dx7VoiceName &name) {
  size_t length = Dx7VoiceNameLength;
  for (size_t character = 0; character < Dx7VoiceNameLength; ++character) {
    const uint8_t value = voiceData[Dx7VoiceNameOffset + character];
    name[character] =
        value >= 0x20U && value <= 0x7EU ? static_cast<char>(value) : ' ';
  }
  while (length > 0 && name[length - 1] == ' ') {
    --length;
  }
  name[length] = '\0';
  if (length == 0) {
    npf_snprintf(name.data(), name.size(), "PATCH %02u",
                 static_cast<unsigned>(patchIndex + 1));
  }
}

Dx7BankParseResult ValidateDx7BankSysex(etl::span<const uint8_t> data) {
  if (data.size() != Dx7BankSysexSize) {
    return Dx7BankParseResult::BadSize;
  }
  if (!CheckDx7BankHeader(data.data()) || data[Dx7BankSysexSize - 1] != 0xF7U) {
    return Dx7BankParseResult::BadHeader;
  }

  uint32_t checksumSum = 0;
  for (size_t index = 0; index < Dx7BankPayloadSize; ++index) {
    const uint8_t value = data[Dx7BankHeaderSize + index];
    if ((value & 0x80U) != 0U) {
      return Dx7BankParseResult::BadData;
    }
    checksumSum += value;
  }
  const uint8_t checksum = data[Dx7BankHeaderSize + Dx7BankPayloadSize];
  if ((checksum & 0x80U) != 0U) {
    return Dx7BankParseResult::BadData;
  }
  const uint8_t expectedChecksum =
      static_cast<uint8_t>((128U - (checksumSum & 0x7FU)) & 0x7FU);
  if (checksum != expectedChecksum) {
    return Dx7BankParseResult::BadChecksum;
  }
  return Dx7BankParseResult::Ok;
}

void ConvertDx7BankSysex(etl::span<const uint8_t> data, Dx7PatchBank &patches,
                         Dx7VoiceNameBank &names) {
  for (size_t patchIndex = 0; patchIndex < Dx7BankPatchCount; ++patchIndex) {
    const uint8_t *voice =
        data.data() + Dx7BankHeaderSize + (patchIndex * Dx7PatchDataSize);
    ConvertDx7Patch(voice, patches[patchIndex]);
    ExtractDx7VoiceName(voice, patchIndex, names[patchIndex]);
  }
}

Dx7BankParseResult ParseDx7BankSysex(etl::span<const uint8_t> data,
                                     Dx7PatchBank &patches,
                                     Dx7VoiceNameBank &names) {
  const Dx7BankParseResult validated = ValidateDx7BankSysex(data);
  if (validated != Dx7BankParseResult::Ok) {
    return validated;
  }
  ConvertDx7BankSysex(data, patches, names);
  return Dx7BankParseResult::Ok;
}

void ConvertDx7Patch(const uint8_t *dx7Data, Patch &patch) {
  if (!dx7Data) {
    return;
  }

  patch.algorithm = dx7Data[110] & 0x1FU;
  patch.feedback = dx7Data[111] & 0x07U;
  patch.oscKeySync = (dx7Data[111] & 0x08U) != 0U;
  patch.transpose = static_cast<int8_t>(dx7Data[117]) - 24;

  // Global pitch EG: rates at 102-105, levels at 106-109. Clamp to the valid
  // DX7 range so a malformed record can never index the pitch/rate LUTs out
  // of bounds in the voice engine.
  for (size_t i = 0; i < 4; ++i) {
    patch.pitchEg.rates[i] = std::min<uint8_t>(dx7Data[102 + i], 99U);
    patch.pitchEg.levels[i] = std::min<uint8_t>(dx7Data[106 + i], 99U);
  }

  // Global LFO block: 112 speed, 113 delay, 114 pitch-mod depth, 115 amp-mod
  // depth, 116 packs sync (bit 0), waveform (bits 1-3) and pitch-mod
  // sensitivity (bits 4-6).
  patch.lfo.speed = std::min<uint8_t>(dx7Data[112], 99U);
  patch.lfo.delay = std::min<uint8_t>(dx7Data[113], 99U);
  patch.lfo.pitchModDepth = std::min<uint8_t>(dx7Data[114], 99U);
  patch.lfo.ampModDepth = std::min<uint8_t>(dx7Data[115], 99U);
  const uint8_t lfoByte = dx7Data[116];
  patch.lfo.sync = (lfoByte & 0x01U) != 0U;
  patch.lfo.waveform = (lfoByte >> 1) & 0x07U;
  patch.lfo.pitchModSensitivity = (lfoByte >> 4) & 0x07U;

  for (size_t op = 0; op < OperatorCount; ++op) {
    const size_t baseOffset = op * 17;
    const uint8_t *opData = &dx7Data[baseOffset];
    const uint8_t curves = opData[11];
    const uint8_t detuneRate = opData[12];
    const uint8_t touchMod = opData[13];
    const uint8_t frequencyMode = opData[15];
    const uint8_t frequencyCoarse = (frequencyMode >> 1) & 0x1FU;
    const uint8_t frequencyFine = std::min<uint8_t>(opData[16], 99U);

    auto &operatorPatch = patch.operators[op];
    operatorPatch.rates = {opData[0], opData[1], opData[2], opData[3]};
    operatorPatch.levels = {opData[4], opData[5], opData[6], opData[7]};
    operatorPatch.breakPoint = opData[8];
    operatorPatch.leftDepth = opData[9];
    operatorPatch.rightDepth = opData[10];
    operatorPatch.leftCurve = curves & 0x03U;
    operatorPatch.rightCurve = (curves >> 2) & 0x03U;
    operatorPatch.rateScaling = detuneRate & 0x07U;
    operatorPatch.keyVelocitySensitivity = (touchMod >> 2) & 0x07U;
    operatorPatch.outputLevel = std::min<uint8_t>(opData[14], 99U);
    operatorPatch.frequencyCoarse = frequencyCoarse;
    operatorPatch.frequencyFine = frequencyFine;
    operatorPatch.detune = static_cast<int8_t>((detuneRate >> 3) & 0x0FU) - 7;
    operatorPatch.fixedFrequency = (frequencyMode & 0x01U) != 0U;

    setRatioFromDx7(frequencyCoarse, frequencyFine,
                    operatorPatch.ratioNumerator,
                    operatorPatch.ratioDenominator);

    // Precompute the operator's log-frequency offset (Q24) once here so the
    // audio path never needs a pow/log per block. Mirrors libmsfa osc_freq():
    // ratio operators carry log2(ratio) plus a symmetric detune term; fixed
    // operators carry an absolute log-frequency with a one-sided detune term.
    if (operatorPatch.fixedFrequency) {
      int32_t fixedLog =
          (4458616 * (static_cast<int32_t>(frequencyCoarse & 0x03U) * 100 +
                      static_cast<int32_t>(frequencyFine))) >>
          3;
      if (operatorPatch.detune > 0) {
        fixedLog += 13457 * operatorPatch.detune;
      }
      operatorPatch.logFreqOffsetQ24 = fixedLog;
    } else {
      const double ratio = static_cast<double>(operatorPatch.ratioNumerator) /
                           static_cast<double>(operatorPatch.ratioDenominator);
      int32_t ratioLog =
          static_cast<int32_t>(std::lround(std::log2(ratio) * 16777216.0));
      ratioLog += 12606 * operatorPatch.detune;
      operatorPatch.logFreqOffsetQ24 = ratioLog;
    }
  }
}

namespace {

// The built-in preset table (dx7_patches) may span several concatenated
// banks, so it is sized independently of a single sysex bank. The packed
// records live in flash (dx7_patches is const); presets are decoded on
// demand rather than cached, so a large bank set costs no RAM.
constexpr size_t Dx7BuiltInPresetCount = DX7_NUM_PATCHES;

int32_t clampProgram(int32_t program) {
  return std::clamp<int32_t>(program, 0,
                             static_cast<int32_t>(Dx7BuiltInPresetCount - 1));
}

} // namespace

void InitializeBuiltInPatches() {}

size_t BuiltInPatchCount() { return Dx7BuiltInPresetCount; }

const Patch &GetBuiltInPatch(int32_t program) {
  // Decoded into a shared scratch: callers must use the result before the
  // next call (all current callers do).
  static Patch scratch;
  ConvertDx7Patch(dx7_patches[static_cast<size_t>(clampProgram(program))],
                  scratch);
  return scratch;
}

const char *GetBuiltInPatchName(int32_t program) {
  // Shared scratch, same contract as GetBuiltInPatch.
  static Dx7VoiceName scratch;
  const size_t index = static_cast<size_t>(clampProgram(program));
  ExtractDx7VoiceName(dx7_patches[index], index, scratch);
  return scratch.data();
}

const char *GetBuiltInPatchBankName(int32_t program) {
  const size_t bank =
      static_cast<size_t>(clampProgram(program)) / Dx7PresetBankSize;
  return dx7_bank_names[bank];
}

const uint8_t *GetBuiltInPatchRecord(int32_t program) {
  return dx7_patches[static_cast<size_t>(clampProgram(program))];
}

} // namespace fm6
