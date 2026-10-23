/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef PICOTRACKER_MEMORY_SECTIONS_H
#define PICOTRACKER_MEMORY_SECTIONS_H

#if defined(ADV) && !defined(HOST_TEST)
#define PICOTRACKER_FAST_CODE(name) __attribute__((section(".fastcode." name)))
#else
#define PICOTRACKER_FAST_CODE(name)
#endif

#endif
