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

#include "CommandList.h"
#include <algorithm>
#include <stddef.h>

// Keep command entries grouped by displayed mnemonic first letter;
// GetNextAlpha/GetPrevAlpha depend on this ordering.
static TrackerCommand _all[] = {
    TrackerCommand::InstrumentCommandNone,
    TrackerCommand::InstrumentCommandArpeggiator,
    TrackerCommand::InstrumentCommandChance,
    TrackerCommand::InstrumentCommandCrush,
    TrackerCommand::InstrumentCommandDelay,
    TrackerCommand::InstrumentCommandFilterCut,
    TrackerCommand::InstrumentCommandLowPassFilter,
    TrackerCommand::InstrumentCommandFilterResonance,
    TrackerCommand::InstrumentCommandGroove,
    TrackerCommand::InstrumentCommandHop,
    TrackerCommand::InstrumentCommandInstrumentRetrigger,
    TrackerCommand::InstrumentCommandKill,
    TrackerCommand::InstrumentCommandLegato,
    TrackerCommand::InstrumentCommandLoopOfset,
    TrackerCommand::InstrumentCommandMidiCC,
    TrackerCommand::InstrumentCommandMidiChord,
    TrackerCommand::InstrumentCommandMidiPC,
    TrackerCommand::InstrumentCommandPan,
    TrackerCommand::InstrumentCommandPitchFineTune,
    TrackerCommand::InstrumentCommandPlayOfset,
    TrackerCommand::InstrumentCommandPitchSlide,
    TrackerCommand::InstrumentCommandOff,
    TrackerCommand::InstrumentCommandRandom,
    TrackerCommand::InstrumentCommandRetrigger,
    TrackerCommand::InstrumentCommandStop,
    TrackerCommand::InstrumentCommandTable,
    TrackerCommand::InstrumentCommandGateLength,
    TrackerCommand::InstrumentCommandTempo,
    TrackerCommand::InstrumentCommandVelocity,
    TrackerCommand::InstrumentCommandVolume,
};

struct CommandRestriction {
  TrackerCommand command;
  uint8_t allowedContexts;
};

static const CommandRestriction _restrictions[] = {
    {TrackerCommand::InstrumentCommandInstrumentRetrigger,
     static_cast<uint8_t>(CommandContext::Table)},
    {TrackerCommand::InstrumentCommandLegato,
     static_cast<uint8_t>(CommandContext::Phrase)},
    {TrackerCommand::InstrumentCommandTable,
     static_cast<uint8_t>(CommandContext::Phrase)},
};

static constexpr size_t kCommandCount = sizeof(_all) / sizeof(_all[0]);

static char GetCommandGroupLetter(TrackerCommand command) {
  const char *name = TrackerCommand(command).c_str();
  return (name && name[0]) ? name[0] : '\0';
}

static size_t FindCommandIndex(TrackerCommand command) {
  for (size_t i = 0; i < kCommandCount; i++) {
    if (_all[i] == command) {
      return i;
    }
  }
  return kCommandCount;
}

bool CommandList::IsCommandAllowed(TrackerCommand command,
                                   CommandContext context) {
  const uint8_t contextMask = static_cast<uint8_t>(context);
  for (const CommandRestriction &restriction : _restrictions) {
    if (restriction.command == command) {
      return (restriction.allowedContexts & contextMask) != 0;
    }
  }
  return true;
}

// Applies command-specific range limits to parameter values
ushort CommandList::RangeLimitCommandParam(TrackerCommand command,
                                           ushort paramValue) {
  // Each command type can have its own specific range limits
  if (command == TrackerCommand::InstrumentCommandVelocity) {
    // For VEL command, limit the bb part to 0x7F (127) while preserving the aa
    // part
    return (paramValue & 0xFF00) | (paramValue & 0x7F);
  }
  if (command == TrackerCommand::InstrumentCommandPan) {
    // PAN targets the shared panlaw table, which is indexed 0..0xFE.
    return (paramValue & 0xFF00) | std::min<ushort>(paramValue & 0x00FF, 0xFE);
  }
  // Add more command-specific limits here as needed
  // Example:
  // else if (command == TrackerCommand::InstrumentCommandMidiCC) {
  //   // MIDI CC values should also be limited to 0-127
  //   return (paramValue & 0xFF00) | (paramValue & 0x7F);
  // }

  // If no specific limit applies, return the original value
  return paramValue;
}

TrackerCommand CommandList::GetNext(TrackerCommand current,
                                    CommandContext context) {
  const size_t currentIndex = FindCommandIndex(current);
  for (size_t i = currentIndex + 1; i < kCommandCount; i++) {
    if (IsCommandAllowed(_all[i], context)) {
      return _all[i];
    }
  }
  return current;
};

TrackerCommand CommandList::GetPrev(TrackerCommand current,
                                    CommandContext context) {
  const size_t currentIndex = FindCommandIndex(current);
  for (size_t i = currentIndex; i > 1; i--) {
    if (IsCommandAllowed(_all[i - 1], context)) {
      return _all[i - 1];
    }
  }
  return current;
};

TrackerCommand CommandList::GetNextAlpha(TrackerCommand current,
                                         CommandContext context) {
  const size_t currentIndex = FindCommandIndex(current);
  if (currentIndex == kCommandCount) {
    return current;
  }

  const char currentLetter = GetCommandGroupLetter(current);
  size_t groupStart = currentIndex + 1;
  while (groupStart < kCommandCount &&
         GetCommandGroupLetter(_all[groupStart]) == currentLetter) {
    groupStart++;
  }

  while (groupStart < kCommandCount) {
    const char groupLetter = GetCommandGroupLetter(_all[groupStart]);
    size_t groupEnd = groupStart;
    while (groupEnd < kCommandCount &&
           GetCommandGroupLetter(_all[groupEnd]) == groupLetter) {
      if (IsCommandAllowed(_all[groupEnd], context)) {
        return _all[groupEnd];
      }
      groupEnd++;
    }
    groupStart = groupEnd;
  }
  return current;
};

TrackerCommand CommandList::GetPrevAlpha(TrackerCommand current,
                                         CommandContext context) {
  size_t groupStart = FindCommandIndex(current);
  if (groupStart == kCommandCount) {
    return current;
  }

  const char currentLetter = GetCommandGroupLetter(current);
  while (groupStart > 0 &&
         GetCommandGroupLetter(_all[groupStart - 1]) == currentLetter) {
    groupStart--;
  }

  // Index zero is the empty command. Preserve the existing behavior where
  // backwards navigation stops at the first actual command instead of moving
  // back to the empty entry.
  while (groupStart > 1) {
    const size_t groupEnd = groupStart;
    groupStart--;
    const char groupLetter = GetCommandGroupLetter(_all[groupStart]);
    while (groupStart > 1 &&
           GetCommandGroupLetter(_all[groupStart - 1]) == groupLetter) {
      groupStart--;
    }
    for (size_t i = groupStart; i < groupEnd; i++) {
      if (IsCommandAllowed(_all[i], context)) {
        return _all[i];
      }
    }
  }
  return current;
};
