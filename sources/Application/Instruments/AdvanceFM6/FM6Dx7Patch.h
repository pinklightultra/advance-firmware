/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _FM6_DX7_PATCH_H_
#define _FM6_DX7_PATCH_H_

#include "Externals/etl/include/etl/array.h"
#include "Externals/etl/include/etl/span.h"
#include <cstddef>
#include <cstdint>

namespace fm6 {

static constexpr size_t OperatorCount = 6;
static constexpr size_t Dx7PatchDataSize = 128;
// Number of voices in a single DX7 bank sysex; this is fixed by the DX7
// format (a 32-voice bank) and drives sysex validation/parsing sizes
static constexpr size_t Dx7BankPatchCount = 32;
// Number of built-in presets shown per bank in the preset browser. The
// built-in preset table is grouped into Dx7PresetBankSize-voice display banks
// (each 32-voice source bank is split in two) so a whole bank fits on screen
// without scrolling. This is independent of the fixed 32-voice DX7 sysex bank
// format above, and must match DX7_BANK_SIZE in FM6PresetPatches.h.
static constexpr size_t Dx7PresetBankSize = 16;
static constexpr size_t Dx7BankPayloadSize =
    Dx7PatchDataSize * Dx7BankPatchCount;
static constexpr size_t Dx7BankHeaderSize = 6;
static constexpr size_t Dx7BankSysexSize = Dx7BankPayloadSize + 8;
static constexpr size_t Dx7VoiceNameOffset = 118;
static constexpr size_t Dx7VoiceNameLength = 10;
static constexpr size_t Dx7VoiceNameStorageSize = Dx7VoiceNameLength + 1;

using Dx7VoiceName = etl::array<char, Dx7VoiceNameStorageSize>;
using Dx7VoiceNameBank = etl::array<Dx7VoiceName, Dx7BankPatchCount>;

// Returns the nominal DX7 fixed-mode oscillator frequency in millihertz.
// Coarse selects one of four decades and fine moves logarithmically through
// that decade. Detune and runtime pitch modulation are intentionally excluded,
// matching the frequency shown while editing coarse/fine on the original DX7.
uint32_t Dx7FixedFrequencyMilliHertz(uint8_t coarse, uint8_t fine);

enum class Dx7BankParseResult : uint8_t {
  Ok = 0,
  BadSize,
  BadHeader,
  BadData,
  BadChecksum,
  // Not produced by the parser itself; used by callers that read a bank
  // from a file before validating it.
  ReadError,
};

struct OperatorPatch {
  etl::array<uint8_t, 4> rates{};
  etl::array<uint8_t, 4> levels{};
  uint8_t breakPoint = 0;
  uint8_t leftDepth = 0;
  uint8_t rightDepth = 0;
  uint8_t leftCurve = 0;
  uint8_t rightCurve = 0;
  uint8_t rateScaling = 0;
  uint8_t keyVelocitySensitivity = 0;
  uint8_t outputLevel = 0;
  uint8_t ratioNumerator = 1;
  uint8_t ratioDenominator = 1;
  uint8_t frequencyCoarse = 0;
  uint8_t frequencyFine = 0;
  int8_t detune = 0;
  bool fixedFrequency = false;
  // Precomputed log-frequency contribution (Q24, 1<<24 == one octave) so the
  // voice engine can resolve pitch with a single integer add per block. For a
  // ratio operator this is log2(ratio) plus detune, added to the note's
  // log-frequency; for a fixed operator it is the absolute log-frequency
  // (including detune) and the note is ignored.
  int32_t logFreqOffsetQ24 = 0;
};

// Global pitch envelope generator (PEG): four rates and four levels that
// modulate the pitch of every operator in unison. Levels default to 50,
// which maps to a zero pitch offset (a flat, inaudible envelope).
struct PitchEgPatch {
  etl::array<uint8_t, 4> rates{};
  etl::array<uint8_t, 4> levels{{50, 50, 50, 50}};
};

// Global low-frequency oscillator (LFO). In the reference engine the LFO
// only modulates pitch (vibrato); amplitude modulation depth is stored but
// unused, so it is kept here for round-tripping without being applied.
struct LfoPatch {
  uint8_t speed = 35;
  uint8_t delay = 0;
  uint8_t pitchModDepth = 0;
  uint8_t ampModDepth = 0;
  uint8_t waveform = 0;
  uint8_t pitchModSensitivity = 0;
  bool sync = true;
};

struct Patch {
  uint8_t algorithm = 0;
  uint8_t feedback = 0;
  int8_t transpose = 0;
  bool oscKeySync = true;
  PitchEgPatch pitchEg{};
  LfoPatch lfo{};
  etl::array<OperatorPatch, OperatorCount> operators{};
};

using Dx7PatchBank = etl::array<Patch, Dx7BankPatchCount>;

void ConvertDx7Patch(const uint8_t *dx7Data, Patch &patch);
// Validates the first Dx7BankHeaderSize bytes of a bank sysex.
bool CheckDx7BankHeader(const uint8_t *header);
// Extracts the printable voice name from a Dx7PatchDataSize-byte voice
// record, falling back to "PATCH nn" when the stored name is blank.
void ExtractDx7VoiceName(const uint8_t *voiceData, size_t patchIndex,
                         Dx7VoiceName &name);
// Checks size, header, data-byte range, checksum and terminator without
// writing anything, so callers can validate before touching bank state.
Dx7BankParseResult ValidateDx7BankSysex(etl::span<const uint8_t> data);
// Converts an already-validated bank sysex; cannot fail.
void ConvertDx7BankSysex(etl::span<const uint8_t> data, Dx7PatchBank &patches,
                         Dx7VoiceNameBank &names);
Dx7BankParseResult ParseDx7BankSysex(etl::span<const uint8_t> data,
                                     Dx7PatchBank &patches,
                                     Dx7VoiceNameBank &names);
void InitializeBuiltInPatches();
size_t BuiltInPatchCount();
const Patch &GetBuiltInPatch(int32_t program);
const char *GetBuiltInPatchName(int32_t program);
// Name of the source sysex bank a built-in preset came from (file basename
// without extension). Banks are exactly Dx7BankPatchCount voices each.
const char *GetBuiltInPatchBankName(int32_t program);
// Raw 128-byte packed voice record of a built-in preset, for previewing
// through the same path as records staged from a sysex bank.
const uint8_t *GetBuiltInPatchRecord(int32_t program);

} // namespace fm6

#endif
