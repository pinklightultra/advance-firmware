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

#ifndef _CREDITS_TEXT_H_
#define _CREDITS_TEXT_H_

#include <stdint.h>

// Lines are drawn at most SCREEN_WIDTH chars wide; wrap manually.
// The dialog window is 28 columns wide, so keep each line <= 28 chars.
static const char *const kCreditsLines[] = {
    "picoTracker Advance",
    "===================",
    "",
    "A hardware music tracker",
    "firmware based on",
    "LittleGPTracker.",
    "",
    "Copyright (c) 2018",
    "  Discodirt",
    "Copyright (c) 2024",
    "  xiphonics, inc.",
    "",
    "Third-party code:",
    "* CSID (cRSID SID emu):",
    "  Hermit / Mihaly Horvath",
    "  WTF license",
    "",
    "* Braids (mutable instr):",
    "  Emilie Gillet",
    "  MIT license",
    "",
    "* MSFA / libmsfa (DX7):",
    "  Raph Levien",
    "  Apache License 2.0",
    "",
    "",
    "Special thanks to:",
    "* Marc Resibois ak Nostromo",
    "* The LittleGPTracker",
    "  community",
    "* Mikael Lundgren aka",
    "  The Introvert",
    "  www.theintrovert.net",
    "* All beta testers",
    "* The open source",
    "  community",
    "",
    "Made with <3 for",
    "music makers.",
    "",
    "Enjoy and make",
    "great music!",
};

static constexpr uint32_t kCreditsLineCount =
    sizeof(kCreditsLines) / sizeof(kCreditsLines[0]);

#endif
