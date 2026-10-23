/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _COMMAND_LIST_H_
#define _COMMAND_LIST_H_

#include "Foundation/Types/Types.h"

enum class CommandContext : uint8_t {
  Phrase = 0x01,
  Table = 0x02,
  All = Phrase | Table,
};

class CommandList {
public:
  static TrackerCommand GetNext(TrackerCommand current,
                                CommandContext context = CommandContext::All);
  static TrackerCommand GetPrev(TrackerCommand current,
                                CommandContext context = CommandContext::All);
  static TrackerCommand
  GetNextAlpha(TrackerCommand current,
               CommandContext context = CommandContext::All);
  static TrackerCommand
  GetPrevAlpha(TrackerCommand current,
               CommandContext context = CommandContext::All);

  // Commands are available everywhere unless listed in the context restriction
  // table. Add future phrase/table restrictions there so every navigation mode
  // observes the same policy.
  static bool IsCommandAllowed(TrackerCommand command, CommandContext context);

  // Applies command-specific range limits to parameter values
  // Currently handles:
  // - VEL: Ensures MIDI velocity values don't exceed 127 (0x7F)
  // Can be extended to handle other commands in the future
  // Returns the range-limited parameter value
  static ushort RangeLimitCommandParam(TrackerCommand command,
                                       ushort paramValue);
};
#endif
