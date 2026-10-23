/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Instruments/AdvanceFM6/FM6Dx7Patch.h"
#include "Externals/etl/include/etl/array.h"
#include <cstring>

namespace {

etl::array<uint8_t, fm6::Dx7BankSysexSize>
makeBank(const char *firstName = "E.PIANO 1") {
  etl::array<uint8_t, fm6::Dx7BankSysexSize> data{};
  data[0] = 0xF0;
  data[1] = 0x43;
  data[2] = 0x00;
  data[3] = 0x09;
  data[4] = 0x20;
  data[5] = 0x00;
  data.back() = 0xF7;
  for (size_t patch = 0; patch < fm6::Dx7BankPatchCount; ++patch) {
    uint8_t *voice = data.data() + 6 + patch * fm6::Dx7PatchDataSize;
    voice[110] = static_cast<uint8_t>(patch & 0x1FU);
    voice[117] = 24;
    const char *name = patch == 0 ? firstName : "VOICE     ";
    std::memcpy(voice + 118, name, fm6::Dx7VoiceNameLength);
  }
  uint32_t sum = 0;
  for (size_t index = 0; index < fm6::Dx7BankPayloadSize; ++index) {
    sum += data[6 + index];
  }
  data[6 + fm6::Dx7BankPayloadSize] =
      static_cast<uint8_t>((128U - (sum & 0x7FU)) & 0x7FU);
  return data;
}

} // namespace

TEST_CASE("DX7 fixed oscillator frequency follows coarse decades and fine") {
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(0, 0) == 1000U);
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(1, 0) == 10000U);
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(2, 0) == 100000U);
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(3, 0) == 1000000U);

  // Fine is logarithmic within the selected decade: 2/99 is 977.237 Hz.
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(2, 99) == 977237U);

  // The packed field is five bits wide, but fixed mode uses its low two bits.
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(4, 0) == 1000U);
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(31, 99) == 9772372U);
  CHECK(fm6::Dx7FixedFrequencyMilliHertz(3, 127) == 9772372U);
}

TEST_CASE("DX7 bulk parser accepts a complete bank and extracts names") {
  const auto bytes = makeBank("E.PIANO  ");
  fm6::Dx7PatchBank patches{};
  fm6::Dx7VoiceNameBank names{};

  REQUIRE(fm6::ParseDx7BankSysex(bytes, patches, names) ==
          fm6::Dx7BankParseResult::Ok);
  CHECK(std::strcmp(names[0].data(), "E.PIANO") == 0);
  CHECK(patches[31].algorithm == 31);
}

TEST_CASE("DX7 bulk parser rejects malformed messages") {
  auto bytes = makeBank();
  fm6::Dx7PatchBank patches{};
  fm6::Dx7VoiceNameBank names{};

  CHECK(fm6::ParseDx7BankSysex(
            etl::span<const uint8_t>(bytes.data(), bytes.size() - 1), patches,
            names) == fm6::Dx7BankParseResult::BadSize);
  bytes = makeBank();
  bytes[1] = 0x44;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadHeader);
  bytes = makeBank();
  bytes[0] = 0x00;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadHeader);
  bytes = makeBank();
  bytes[2] = 0x10;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadHeader);
  bytes = makeBank();
  bytes[3] = 0x00;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadHeader);
  bytes = makeBank();
  bytes[4] = 0x10;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadHeader);
  bytes = makeBank();
  bytes.back() = 0x00;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadHeader);
  bytes = makeBank();
  bytes[20] = 0x80;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadData);
  bytes = makeBank();
  bytes[6 + fm6::Dx7BankPayloadSize] ^= 1U;
  CHECK(fm6::ParseDx7BankSysex(bytes, patches, names) ==
        fm6::Dx7BankParseResult::BadChecksum);
}

TEST_CASE("DX7 blank and non-printable names use deterministic fallbacks") {
  auto bytes = makeBank("          ");
  bytes[6 + 118] = 0x01;
  uint32_t sum = 0;
  for (size_t index = 0; index < fm6::Dx7BankPayloadSize; ++index) {
    sum += bytes[6 + index];
  }
  bytes[6 + fm6::Dx7BankPayloadSize] =
      static_cast<uint8_t>((128U - (sum & 0x7FU)) & 0x7FU);
  fm6::Dx7PatchBank patches{};
  fm6::Dx7VoiceNameBank names{};
  REQUIRE(fm6::ParseDx7BankSysex(bytes, patches, names) ==
          fm6::Dx7BankParseResult::Ok);
  CHECK(std::strcmp(names[0].data(), "PATCH 01") == 0);
}

TEST_CASE("DX7 built-in presets resolve patches and names") {
  // Built-in presets are one or more whole 32-voice banks concatenated.
  REQUIRE(fm6::BuiltInPatchCount() >= fm6::Dx7BankPatchCount);
  REQUIRE(fm6::BuiltInPatchCount() % fm6::Dx7BankPatchCount == 0);
  for (size_t program = 0; program < fm6::BuiltInPatchCount(); ++program) {
    const char *name = fm6::GetBuiltInPatchName(static_cast<int32_t>(program));
    REQUIRE(name != nullptr);
    CHECK(name[0] != '\0');
  }
  // Out-of-range programs clamp instead of reading out of bounds.
  CHECK(&fm6::GetBuiltInPatch(-5) == &fm6::GetBuiltInPatch(0));
  CHECK(&fm6::GetBuiltInPatch(99) == &fm6::GetBuiltInPatch(static_cast<int32_t>(
                                         fm6::BuiltInPatchCount() - 1)));
}
