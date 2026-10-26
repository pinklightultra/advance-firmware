/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * DesktopDisplay.h - Software display driver for picoTracker Advance on Desktop
 */

#ifndef _DESKTOP_DISPLAY_H_
#define _DESKTOP_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_WIDTH 720
#define DISPLAY_HEIGHT 720
#define TEXT_WIDTH 33
#define TEXT_HEIGHT 24
#define CHAR_HEIGHT 30
#define CHAR_WIDTH 22
#define DISPLAY_OFFSET 8

typedef enum {
  COLOR_BG,
  COLOR_NORMAL,
  COLOR_HILITE,
  COLOR_HILITE2,
  COLOR_GRAY,
  COLOR_DESERT,
  COLOR_ORANGE,
  COLOR_YELLOW,
  COLOR_WHITE,
  COLOR_MIDNIGHT,
  COLOR_DARK_SLATE_GRAY,
  COLOR_GREEN,
  COLOR_YELLOW_GREEN,
  COLOR_BLUE,
  COLOR_GURU_TXT,
  COLOR_GURU_BG
} color_t;

extern uint16_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
extern bool display_is_dirty;

void display_mark_dirty(void);
void display_set_font_index(uint8_t idx);
void display_set_cursor(uint8_t x, uint8_t y);
void display_putc(char c, bool invert);
void display_print(const char *s, bool invert);
void display_set_foreground(color_t color);
void display_set_background(color_t color);
void display_clear(color_t color);
void display_set_palette_color(int idx, uint16_t rgb565_color);
void display_draw_changed(void);
void display_draw_screen(void);
void display_fill_rect(uint8_t color_index, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height);
void display_draw_mono_bitmap(uint16_t x, uint16_t y, const uint8_t *bitmap,
                              uint16_t width, uint16_t height,
                              uint16_t row_bytes, uint8_t fg_color_index,
                              uint8_t bg_color_index);
void display_draw_mono_bitmap_rgb565(uint16_t x, uint16_t y,
                                     const uint8_t *bitmap, uint16_t width,
                                     uint16_t height, uint16_t row_bytes,
                                     uint16_t fg_color565,
                                     uint16_t bg_color565);
void display_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
uint32_t rgb565_to_abgr8888(uint16_t rgb565);

#ifdef __cplusplus
}
#endif

#endif // _DESKTOP_DISPLAY_H_
