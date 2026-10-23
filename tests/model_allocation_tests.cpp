/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Application/Model/Chain.h"
#include "Application/Model/Phrase.h"
#include "doctest/doctest.h"

TEST_CASE("chain allocation can start at the current chain") {
  Chain chain;

  chain.SetUsed(0x05);

  CHECK(chain.GetNextFrom(0x05) == 0x06);
  CHECK(chain.GetNext() == 0x00);
}

TEST_CASE("chain allocation from current does not wrap") {
  Chain chain;

  chain.SetUsed(0xFE);

  CHECK(chain.GetNextFrom(0xFE) == NO_MORE_CHAIN);
}

TEST_CASE("phrase allocation can start at the current phrase") {
  Phrase phrase;

  phrase.SetUsed(0x05);

  CHECK(phrase.GetNextFrom(0x05) == 0x06);
  CHECK(phrase.GetNext() == 0x00);
}

TEST_CASE("phrase allocation from current does not wrap") {
  Phrase phrase;

  phrase.SetUsed(0xFE);

  CHECK(phrase.GetNextFrom(0xFE) == NO_MORE_PHRASE);
}
