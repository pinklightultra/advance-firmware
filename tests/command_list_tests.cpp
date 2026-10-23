/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Application/Instruments/CommandList.h"
#include "doctest/doctest.h"
#include <initializer_list>

namespace {

using NavigateCommand = TrackerCommand (*)(TrackerCommand, CommandContext);

TrackerCommand FindNavigationEnd(CommandContext context,
                                 NavigateCommand navigate) {
  TrackerCommand current = TrackerCommand::InstrumentCommandNone;
  for (uint16_t i = 0; i <= UINT8_MAX; i++) {
    const TrackerCommand next = navigate(current, context);
    CHECK(CommandList::IsCommandAllowed(next, context));
    if (next == current) {
      return current;
    }
    current = next;
  }

  FAIL("Command navigation did not terminate");
  return current;
}

void CheckReverseNavigation(CommandContext context, NavigateCommand forward,
                            NavigateCommand reverse) {
  TrackerCommand current = FindNavigationEnd(context, forward);
  for (uint16_t i = 0; i <= UINT8_MAX; i++) {
    const TrackerCommand previous = reverse(current, context);
    CHECK(CommandList::IsCommandAllowed(previous, context));
    if (previous == current) {
      return;
    }
    current = previous;
  }

  FAIL("Reverse command navigation did not terminate");
}

} // namespace

TEST_CASE("Commands declare whether phrase and table contexts support them") {
  CHECK_FALSE(CommandList::IsCommandAllowed(
      TrackerCommand::InstrumentCommandInstrumentRetrigger,
      CommandContext::Phrase));
  CHECK(CommandList::IsCommandAllowed(
      TrackerCommand::InstrumentCommandInstrumentRetrigger,
      CommandContext::Table));
  CHECK(CommandList::IsCommandAllowed(TrackerCommand::InstrumentCommandTable,
                                      CommandContext::Phrase));
  CHECK_FALSE(CommandList::IsCommandAllowed(
      TrackerCommand::InstrumentCommandTable, CommandContext::Table));
  CHECK(CommandList::IsCommandAllowed(TrackerCommand::InstrumentCommandLegato,
                                      CommandContext::Phrase));
  CHECK_FALSE(CommandList::IsCommandAllowed(
      TrackerCommand::InstrumentCommandLegato, CommandContext::Table));

  CHECK(CommandList::IsCommandAllowed(TrackerCommand::InstrumentCommandVolume,
                                      CommandContext::Phrase));
  CHECK(CommandList::IsCommandAllowed(TrackerCommand::InstrumentCommandVolume,
                                      CommandContext::Table));
}

TEST_CASE("Command navigation only returns commands supported by its context") {
  for (const CommandContext context :
       {CommandContext::Phrase, CommandContext::Table}) {
    FindNavigationEnd(context, CommandList::GetNext);
    FindNavigationEnd(context, CommandList::GetNextAlpha);
    CheckReverseNavigation(context, CommandList::GetNext, CommandList::GetPrev);
    CheckReverseNavigation(context, CommandList::GetNextAlpha,
                           CommandList::GetPrevAlpha);
  }
}
