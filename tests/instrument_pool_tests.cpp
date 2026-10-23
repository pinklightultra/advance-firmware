/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Externals/etl/include/etl/variant_pool.h"

namespace {

struct DummySampleInstrument {
  explicit DummySampleInstrument(int id = 0) : id_(id) {}
  int id_;
};

struct DummyMidiInstrument {
  explicit DummyMidiInstrument(int channel = 0) : channel_(channel) {}
  int channel_;
};

using TestInstrumentPool =
    etl::variant_pool<4, DummySampleInstrument, DummyMidiInstrument>;

} // namespace

TEST_CASE("Variant pool shares capacity across instrument types") {
  TestInstrumentPool pool;

  CHECK(pool.max_size() == 4);
  CHECK(pool.available() == 4);

  CHECK(pool.create<DummySampleInstrument>(1) != nullptr);
  CHECK(pool.create<DummyMidiInstrument>(2) != nullptr);
  CHECK(pool.create<DummySampleInstrument>(3) != nullptr);
  CHECK(pool.create<DummyMidiInstrument>(4) != nullptr);

  CHECK(pool.available() == 0);
  CHECK(pool.full());
  CHECK(pool.create<DummySampleInstrument>(5) == nullptr);
}

TEST_CASE("Variant pool reuses freed slots across instrument types") {
  TestInstrumentPool pool;

  DummySampleInstrument *sample = pool.create<DummySampleInstrument>(11);
  REQUIRE(sample != nullptr);
  CHECK(sample->id_ == 11);

  pool.destroy(sample);
  CHECK(pool.available() == 4);

  DummyMidiInstrument *midi = pool.create<DummyMidiInstrument>(7);
  REQUIRE(midi != nullptr);
  CHECK(midi->channel_ == 7);
  CHECK(pool.available() == 3);
}
