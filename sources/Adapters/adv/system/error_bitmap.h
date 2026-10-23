/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#ifndef ERROR_BITMAP_H
#define ERROR_BITMAP_H

#include <stdint.h>

typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t rowBytes;
  const uint8_t *data;
} ErrorBitmap;

#endif
