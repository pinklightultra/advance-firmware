#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.

"""Generate a polished screenmap diagram for the picoTracker Advance user manual.

Creates a flowchart showing all screens with low-res thumbnails embedded in each box,
connected by styled arrows. Adds the new Voice screen to the right of Instrument.
"""

import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

# --- Configuration ---

IMG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "..", "advance-edition", "content", "static", "image")
OUTPUT = os.path.join(IMG_DIR, "screenmap.png")

# Colors - professional dark theme with vibrant accents
BG_COLOR = (255, 255, 255)
BOX_BG = (26, 30, 38)
BOX_BORDER = (55, 63, 78)
TEXT_COLOR = (225, 230, 240)
ARROW_COLOR = (80, 160, 255)
ARROW_GLOW = (40, 90, 180)
TITLE_COLOR = (60, 70, 90)

# Accent colors per screen (top-bar highlights)
ACCENT_COLORS = {
    "Device":     (60, 210, 110),
    "Project":    (255, 170, 40),
    "Song":       (60, 220, 180),
    "Mixer":      (190, 110, 255),
    "Chain":      (80, 150, 255),
    "Phrase":     (255, 130, 90),
    "Instrument": (255, 210, 60),
    "Voice":      (170, 90, 255),
    "Groove":     (80, 220, 80),
    "Table":      (140, 140, 200),
}

# Layout constants
BOX_W = 320
BOX_H = 360
THUMB_W = 290
THUMB_H = 230
LABEL_H = 110
H_GAP = 90
V_GAP = 100
PADDING = 60

# Screen image filenames
SCREEN_FILES = {
    "Device":     "device-screen-small.png",
    "Project":    "project-screen-small.png",
    "Song":       "song-screen-small.png",
    "Mixer":      "mixer-screen-small.png",
    "Chain":      "chain-screen-small.png",
    "Phrase":     "phrase-screen-small.png",
    "Instrument": "instrument-screen-small.png",
    "Voice":      "voice-screen-small.png",
    "Groove":     "groove-screen-small.png",
    "Table":      "table-screen-small.png",
}

# Grid positions: (col, row) - higher row = lower on screen
GRID = {
    "Device":     (0, 0),
    "Project":    (0, 1),
    "Song":       (0, 2),
    "Mixer":      (0, 3),
    "Chain":      (1, 2),
    "Phrase":     (2, 2),
    "Groove":     (2, 0),
    "Instrument": (3, 2),
    "Voice":      (4, 2),
    "Table-L":    (2, 3),
    "Table-R":    (3, 3),
}

# Connections: pairs of screen names (bidirectional arrows)
CONNECTIONS = [
    ("Device", "Project"),
    ("Project", "Song"),
    ("Song", "Mixer"),
    ("Song", "Chain"),
    ("Chain", "Phrase"),
    ("Phrase", "Groove"),
    ("Phrase", "Instrument"),
    ("Instrument", "Voice"),
    ("Phrase", "Table-L"),
    ("Instrument", "Table-R"),
    ("Table-L", "Table-R"),
]


def load_screen_thumb(name):
    """Load a screen image and resize to thumbnail dimensions with pixelation effect."""
    path = os.path.join(IMG_DIR, SCREEN_FILES[name])
    img = Image.open(path).convert("RGB")

    # Downscale then upscale for "sketched / low-res" look
    scale_factor = 0.35
    small_w = max(1, int(THUMB_W * scale_factor))
    small_h = max(1, int(THUMB_H * scale_factor))
    img = img.resize((small_w, small_h), Image.BOX)
    img = img.resize((THUMB_W, THUMB_H), Image.NEAREST)

    return img


def get_box_center(grid_pos):
    col, row = grid_pos
    cx = PADDING + col * (BOX_W + H_GAP) + BOX_W // 2
    cy = PADDING + row * (BOX_H + V_GAP) + BOX_H // 2
    return cx, cy


def get_box_rect(grid_pos):
    col, row = grid_pos
    x = PADDING + col * (BOX_W + H_GAP)
    y = PADDING + row * (BOX_H + V_GAP)
    return (x, y, BOX_W, BOX_H)


def _make_rounded_rect_mask(w, h, r):
    """Create an L-mode mask image for a rounded rectangle."""
    mask = Image.new("L", (w, h), 0)
    md = ImageDraw.Draw(mask)
    # Use a full-coverage approach: draw overlapping shapes
    md.rectangle([r, 0, w - r, h], fill=255)
    md.rectangle([0, r, w, h - r], fill=255)
    md.pieslice([0, 0, 2 * r, 2 * r], 180, 270, fill=255)
    md.pieslice([w - 2 * r, 0, w, 2 * r], 270, 360, fill=255)
    md.pieslice([0, h - 2 * r, 2 * r, h], 90, 180, fill=255)
    md.pieslice([w - 2 * r, h - 2 * r, w, h], 0, 90, fill=255)
    return mask


def draw_rounded_rect(target_img, xy, radius, fill, outline=None):
    """Draw a rounded rectangle with optional outline."""
    x0, y0, w, h = xy
    if outline:
        ow, oh = w + 2, h + 2
        mask = _make_rounded_rect_mask(ow, oh, radius + 1)
        outline_img = Image.new("RGB", (ow, oh), outline)
        target_img.paste(outline_img, (x0 - 1, y0 - 1), mask)
        mask = _make_rounded_rect_mask(w, h, radius)
        fill_img = Image.new("RGB", (w, h), fill)
        target_img.paste(fill_img, (x0, y0), mask)
    else:
        mask = _make_rounded_rect_mask(w, h, radius)
        fill_img = Image.new("RGB", (w, h), fill)
        target_img.paste(fill_img, (x0, y0), mask)


def draw_arrow(draw, x1, y1, x2, y2, color, arrow_size=16):
    """Draw a bidirectional arrow between two points."""
    dx = x2 - x1
    dy = y2 - y1
    length = (dx * dx + dy * dy) ** 0.5
    if length == 0:
        return

    nx, ny = dx / length, dy / length

    # Glow layer
    draw.line([(x1, y1), (x2, y2)], fill=ARROW_GLOW, width=6)
    draw.line([(x1, y1), (x2, y2)], fill=color, width=3)

    # Arrowhead at start
    ax1 = x1 + nx * arrow_size
    ay1 = y1 + ny * arrow_size
    perp_x, perp_y = -ny * arrow_size * 0.45, nx * arrow_size * 0.45
    draw.polygon([
        (x1, y1),
        (ax1 + perp_x, ay1 + perp_y),
        (ax1 - perp_x, ay1 - perp_y),
    ], fill=color)

    # Arrowhead at end
    ax2 = x2 - nx * arrow_size
    ay2 = y2 - ny * arrow_size
    draw.polygon([
        (x2, y2),
        (ax2 + perp_x, ay2 + perp_y),
        (ax2 - perp_x, ay2 - perp_y),
    ], fill=color)


def get_connection_points(name1, name2):
    """Calculate the edge points for drawing an arrow between two boxes."""
    rect1 = get_box_rect(GRID[name1])
    rect2 = get_box_rect(GRID[name2])
    cx1, cy1 = get_box_center(GRID[name1])
    cx2, cy2 = get_box_center(GRID[name2])

    x1, y1, x2, y2 = cx1, cy1, cx2, cy2

    dx = abs(cx2 - cx1)
    dy = abs(cy2 - cy1)

    margin = 4  # inset from edge for rounded corners

    if dx >= dy:
        if cx2 > cx1:
            x1 = rect1[0] + rect1[2] - margin
            x2 = rect2[0] + margin
        else:
            x1 = rect1[0] + margin
            x2 = rect2[0] + rect2[2] - margin
    else:
        if cy2 > cy1:
            y1 = rect1[1] + rect1[3] - margin
            y2 = rect2[1] + margin
        else:
            y1 = rect1[1] + margin
            y2 = rect2[1] + rect2[3] - margin

    return (x1, y1), (x2, y2)


def get_screen_name(label):
    if label.startswith("Table"):
        return "Table"
    return label


def main():
    max_col = max(g[0] for g in GRID.values())
    max_row = max(g[1] for g in GRID.values())
    canvas_w = PADDING * 2 + (max_col + 1) * BOX_W + max_col * H_GAP + 60
    canvas_h = PADDING * 2 + (max_row + 1) * BOX_H + max_row * V_GAP + 100

    img = Image.new("RGB", (canvas_w, canvas_h), BG_COLOR)
    draw = ImageDraw.Draw(img)

    # Load fonts
    font_bold = None
    font_regular = None
    font_paths = [
        ("/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf",
         "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf"),
        ("/usr/share/fonts/rsms-inter-fonts/InterDisplay-Bold.ttf",
         "/usr/share/fonts/rsms-inter-fonts/InterDisplay-Regular.ttf"),
        ("/usr/share/fonts/open-sans/OpenSans-Bold.ttf",
         "/usr/share/fonts/open-sans/OpenSans-Regular.ttf"),
    ]
    for bold_path, reg_path in font_paths:
        if os.path.exists(bold_path) and os.path.exists(reg_path):
            try:
                font_bold = ImageFont.truetype(bold_path, 52)
                font_regular = ImageFont.truetype(reg_path, 30)
            except Exception:
                continue
            break

    if not font_bold:
        font_bold = ImageFont.load_default()
        font_regular = font_bold

    try:
        title_font = ImageFont.truetype(
            "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf", 28)
    except Exception:
        title_font = font_bold

    # Title - not used for now
    title = ""
    bbox = draw.textbbox((0, 0), title, font=title_font)
    tw = bbox[2] - bbox[0]
    draw.text(((canvas_w - tw) // 2, 16), title, fill=TITLE_COLOR, font=title_font)

    # Load all thumbnails
    thumbs = {}
    for name in GRID:
        screen_name = get_screen_name(name)
        thumbs[name] = load_screen_thumb(screen_name)

    # Draw connections first (behind boxes)
    for name1, name2 in CONNECTIONS:
        (x1, y1), (x2, y2) = get_connection_points(name1, name2)
        draw_arrow(draw, x1, y1, x2, y2, ARROW_COLOR)

    # Draw boxes
    for label, grid_pos in GRID.items():
        screen_name = get_screen_name(label)
        rect = get_box_rect(grid_pos)
        x0, y0, w, h = rect
        accent = ACCENT_COLORS.get(screen_name, (120, 180, 255))
        R = 16  # corner radius

        # Box with outline
        draw_rounded_rect(img, rect, R, BOX_BG, BOX_BORDER)

        # Accent top bar with rounded corners
        accent_h = 5
        # Left and right caps for accent bar
        draw.pieslice([x0, y0, x0 + 2 * R, y0 + 2 * R], 180, 270, fill=accent)
        draw.pieslice([x0 + w - 2 * R, y0, x0 + w, y0 + 2 * R], 270, 360, fill=accent)
        draw.rectangle([x0 + R, y0, x0 + w - R, y0 + accent_h], fill=accent)

        # Screen thumbnail with inset border
        thumb = thumbs[label]
        thumb_x = x0 + (w - THUMB_W) // 2
        thumb_y = y0 + 16
        # Dark inset border behind thumbnail
        draw.rectangle([thumb_x - 2, thumb_y - 2,
                        thumb_x + THUMB_W + 2, thumb_y + THUMB_H + 2],
                       fill=(15, 17, 22))
        img.paste(thumb, (thumb_x, thumb_y))

        # Label area background (bottom of box)
        label_bg_y = y0 + h - LABEL_H
        # Draw a subtle gradient-like separator line
        draw.line([(x0 + R + 4, label_bg_y - 2), (x0 + w - R - 4, label_bg_y - 2)],
                  fill=BOX_BORDER, width=1)

        # Screen label text
        if label.startswith("Table"):
            suffix = label.split("-")[1]
            display_name = f"Table ({'P' if suffix == 'L' else 'I'})"
        else:
            display_name = screen_name

        bbox = draw.textbbox((0, 0), display_name, font=font_bold)
        lw = bbox[2] - bbox[0]
        lh = bbox[3] - bbox[1]
        text_x = x0 + (w - lw) // 2
        text_y = label_bg_y + (LABEL_H - lh) // 2 - 2

        # Subtle text glow
        draw.text((text_x + 1, text_y + 1), display_name, fill=ARROW_GLOW, font=font_bold)
        draw.text((text_x, text_y), display_name, fill=TEXT_COLOR, font=font_bold)

    # Save with good quality
    img.save(OUTPUT, "PNG")
    print(f"Saved screenmap to {OUTPUT} ({img.size[0]}x{img.size[1]})")


if __name__ == "__main__":
    main()
