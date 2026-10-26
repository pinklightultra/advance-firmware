/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * platform.h - Desktop Platform header for picoTracker Advance
 */

#ifndef _PLATFORM_DESKTOP_H_
#define _PLATFORM_DESKTOP_H_

#include "System/Process/SysMutex.h"
#include <cstdint>

void platform_init();
void platform_reboot();
void platform_bootloader();
SysMutex *platform_mutex();
uint32_t millis(void);
uint32_t micros(void);
void platform_brightness(uint8_t value);
int16_t battery_health();
uint32_t platform_get_rand();

#endif // _PLATFORM_DESKTOP_H_
