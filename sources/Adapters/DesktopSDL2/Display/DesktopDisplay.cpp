/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * DesktopDisplay.cpp - Software display implementation using Advance font for Desktop
 */

#include "DesktopDisplay.h"
#include "Adapters/adv/display/font.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <algorithm>

uint16_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT] = {0};
bool display_is_dirty = true;

void display_mark_dirty(void) {
  display_is_dirty = true;
}

static color_t screen_bg_color = COLOR_BG;
static color_t screen_fg_color = COLOR_NORMAL;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t screen[TEXT_HEIGHT * TEXT_WIDTH] = {0};
static uint8_t colors[TEXT_HEIGHT * TEXT_WIDTH] = {0};
static uint8_t ui_font_index = 0;

static uint8_t changed[TEXT_HEIGHT * TEXT_WIDTH / 8] = {0};
#define SetBit(A, k) (A[(k) / 8] |= (1 << ((k) % 8)))
#define ClearBit(A, k) (A[(k) / 8] &= ~(1 << ((k) % 8)))
#define TestBit(A, k) (A[(k) / 8] & (1 << ((k) % 8)))

uint32_t rgb565_to_abgr8888(uint16_t rgb565) {
  uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
  uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
  uint8_t b = (rgb565 & 0x1F) << 3;
  return 0xFF000000 | (b << 16) | (g << 8) | r;
}

static inline uint16_t abgr8888_to_bgr565(uint32_t color) {
  uint16_t b = (uint16_t)((color >> 19) & 0x1F);
  uint16_t g = (uint16_t)((color >> 10) & 0x3F);
  uint16_t r = (uint16_t)((color >> 3) & 0x1F);
  return (uint16_t)((b << 11) | (g << 5) | r);
}

static uint32_t palette[16] = {
    0xFF000000, // 0x0000
    0xFF65CC92, // 0x49E5
    0xFF2E92B2, // 0xB926
    0xFF71C5C7, // 0xE371
    0xFF73F19C, // 0x9CF3
    0xFF2494A3, // 0xA324
    0xFF46C3EC, // 0xEC46
    0xFF0DF7F7, // 0xF70D
    0xFFFFFFFF, // 0xFFFF
    0xFF269019, // 0x1926
    0xFF49A12A, // 0x2A49
    0xFF438244, // 0x4443
    0xFF6496A6, // 0xA664
    0xFFB00200, // 0x02B0
    0xFF1E9635, // 0x351E
    0xFFFD6DB6, // 0xB6FD
};

void display_set_font_index(uint8_t idx) { ui_font_index = idx; }

void display_set_cursor(uint8_t x, uint8_t y) {
  cursor_x = x;
  cursor_y = y;
}

void display_putc(char c, bool invert) {
  if (cursor_x < 0 || cursor_x >= TEXT_WIDTH || cursor_y < 0 || cursor_y >= TEXT_HEIGHT) {
    return;
  }
  int idx = cursor_y * TEXT_WIDTH + cursor_x;
  uint8_t uc = (uint8_t)c;
  if (uc >= 32) {
    screen[idx] = uc - 32;
    SetBit(changed, idx);
    if (invert) {
      colors[idx] = ((screen_bg_color & 0xf) << 4) | (screen_fg_color & 0xf);
    } else {
      colors[idx] = ((screen_fg_color & 0xf) << 4) | (screen_bg_color & 0xf);
    }
  }
}

void display_print(const char *str, bool invert) {
  if (!str) return;
  char c;
  while ((c = *str++)) {
    display_putc(c, invert);
    cursor_x++;
    if (cursor_x >= TEXT_WIDTH) {
      cursor_x = 0;
      cursor_y++;
      if (cursor_y >= TEXT_HEIGHT) break;
    }
  }
}

void display_set_foreground(color_t color) { screen_fg_color = color; }
void display_set_background(color_t color) { screen_bg_color = color; }

void display_clear(color_t color) {
  int size = TEXT_WIDTH * TEXT_HEIGHT;
  memset(screen, 0, size);
  memset(colors, (color & 0xF) | ((color & 0xF) << 4), size);
  cursor_x = 0;
  cursor_y = 0;
  uint16_t color565 = abgr8888_to_bgr565(palette[color & 0xF]);
  for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; ++i) {
    framebuffer[i] = color565;
  }
  memset(changed, 0, sizeof(changed));
  display_is_dirty = true;
}

void display_set_palette_color(int idx, uint16_t rgb565_color) {
  if (idx >= 0 && idx < 16) {
    palette[idx] = rgb565_to_abgr8888(rgb565_color);
    display_is_dirty = true;
  }
}

void display_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  display_is_dirty = true;
  for (uint8_t h = y; h < y + height && h < TEXT_HEIGHT; h++) {
    for (uint8_t w = x; w < x + width && w < TEXT_WIDTH; w++) {
      uint32_t cellIdx = h * TEXT_WIDTH + w;
      uint8_t char_index = screen[cellIdx];
      if (char_index >= FONT_CHAR_COUNT) {
        char_index = 0;
      }
      uint32_t bg_color = palette[colors[cellIdx] & 0xF];
      uint32_t fg_color = palette[(colors[cellIdx] >> 4) & 0xF];
      uint16_t bg565 = abgr8888_to_bgr565(bg_color);
      uint16_t fg565 = abgr8888_to_bgr565(fg_color);

      int dest_x = w * CHAR_WIDTH + DISPLAY_OFFSET;
      int dest_y = h * CHAR_HEIGHT;

      for (int py = 0; py < CHAR_HEIGHT; py++) {
        int py_dest = dest_y + py;
        if (py_dest >= DISPLAY_HEIGHT) break;
        uint16_t *line = &framebuffer[py_dest * DISPLAY_WIDTH + dest_x];
        for (int px = 0; px < CHAR_WIDTH; px++) {
          if (dest_x + px < DISPLAY_WIDTH) {
            line[px] = FONT[char_index][py][px] ? fg565 : bg565;
          }
        }
      }
    }
  }
}

void display_draw_changed(void) {
  for (int idx = 0; idx < TEXT_HEIGHT * TEXT_WIDTH; idx++) {
    if (TestBit(changed, idx)) {
      uint32_t fg_color = palette[(colors[idx] >> 4) & 0xF];
      uint32_t bg_color = palette[colors[idx] & 0xF];

      ClearBit(changed, idx);
      uint16_t y = idx / TEXT_WIDTH;
      uint16_t x = idx % TEXT_WIDTH;

      int height = 1;
      for (int probe_y = y + 1; probe_y < TEXT_HEIGHT; probe_y++) {
        int probe_idx = probe_y * TEXT_WIDTH + x;
        if (TestBit(changed, probe_idx) &&
            fg_color == palette[(colors[probe_idx] >> 4) & 0xF] &&
            bg_color == palette[colors[probe_idx] & 0xF]) {
          ClearBit(changed, probe_idx);
          height++;
          continue;
        }
        break;
      }

      int16_t width = 1;
      for (int probe_x = x + 1; probe_x < TEXT_WIDTH; probe_x++) {
        for (int probe_y = y; probe_y < y + height; probe_y++) {
          int probe_idx = probe_y * TEXT_WIDTH + probe_x;
          if (!TestBit(changed, probe_idx) ||
              fg_color != palette[(colors[probe_idx] >> 4) & 0xF] ||
              bg_color != palette[colors[probe_idx] & 0xF]) {
            for (int undo_y = y; undo_y < probe_y; undo_y++) {
              SetBit(changed, undo_y * TEXT_WIDTH + probe_x);
            }
            goto end_block;
          }
          ClearBit(changed, probe_idx);
        }
        width++;
      }
    end_block:
      display_draw_region(x, y, width, height);
    }
  }
}

void display_draw_screen(void) {
  display_draw_region(0, 0, TEXT_WIDTH, TEXT_HEIGHT);
}

void display_fill_rect(uint8_t color_index, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height) {
  if (width == 0 || height == 0) return;
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
  if (x + width > DISPLAY_WIDTH) width = DISPLAY_WIDTH - x;
  if (y + height > DISPLAY_HEIGHT) height = DISPLAY_HEIGHT - y;

  display_is_dirty = true;
  uint16_t color565 = abgr8888_to_bgr565(palette[color_index & 0xF]);

  for (uint16_t j = y; j < y + height; j++) {
    uint16_t *line = &framebuffer[j * DISPLAY_WIDTH + x];
    for (uint16_t i = 0; i < width; i++) {
      line[i] = color565;
    }
  }
}

void display_draw_mono_bitmap(uint16_t x, uint16_t y, const uint8_t *bitmap,
                              uint16_t width, uint16_t height,
                              uint16_t row_bytes, uint8_t fg_color_index,
                              uint8_t bg_color_index) {
  uint16_t fg_color565 = abgr8888_to_bgr565(palette[fg_color_index & 0xF]);
  uint16_t bg_color565 = abgr8888_to_bgr565(palette[bg_color_index & 0xF]);
  display_draw_mono_bitmap_rgb565(x, y, bitmap, width, height, row_bytes,
                                  fg_color565, bg_color565);
}

void display_draw_mono_bitmap_rgb565(uint16_t x, uint16_t y,
                                     const uint8_t *bitmap, uint16_t width,
                                     uint16_t height, uint16_t row_bytes,
                                     uint16_t fg_color565,
                                     uint16_t bg_color565) {
  if (!bitmap) return;
  display_is_dirty = true;
  for (uint16_t r = 0; r < height; r++) {
    if (y + r >= DISPLAY_HEIGHT) break;
    const uint8_t *row = bitmap + (r * row_bytes);
    uint16_t *line = &framebuffer[(y + r) * DISPLAY_WIDTH];
    for (uint16_t c = 0; c < width; c++) {
      if (x + c >= DISPLAY_WIDTH) break;
      bool pixel = (row[c / 8] & (0x80 >> (c % 8))) != 0;
      line[x + c] = pixel ? fg_color565 : bg_color565;
    }
  }
}
