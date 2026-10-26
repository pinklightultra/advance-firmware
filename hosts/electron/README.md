# picoTracker Advance — ROG Ally / Handheld Edition (Electron)

Standalone, high-performance desktop application for **picoTracker Advance v3.0** tailored specifically for the **ASUS ROG Ally**, **Steam Deck** (running Windows), and Windows handhelds.

Modeled after modern handheld trackers like the Dirtywave M8 emulator, this edition delivers a custom 16:9 widescreen layout with dedicated lateral touch wings, full HTML5 Gamepad / XInput integration, low-latency offscreen native audio, and high-framerate frame streaming.

---

## Architecture Overview

```
+---------------------------------------------------------------------------------+
|                       Electron Host (Chromium / Node.js)                         |
|                                                                                 |
|  +--------------------+   +---------------------------+   +------------------+  |
|  |  Left Touch Wing   |   |    Crisp 720x720 Canvas   |   | Right Touch Wing |  |
|  |  - Directional Pad |   |  - Nearest-Neighbor Scaler|   | - Action A / B   |  |
|  |  - NAV (L-Shoulder)|   |  - 1:1 / 16:9 / 4:3 Modes |   | - OPT (R-Shoulder|  |
|  |  - VIEW (Select)   |   |  - Real-time FPS & Status |   | - PLAY / DELETE  |  |
|  +--------------------+   +---------------------------+   +------------------+  |
|                                                                                 |
|  HTML5 Gamepad API (60Hz XInput Polling) + Multi-Touch Pointer Capture API     |
+----------------------------------------+----------------------------------------+
                                         |
                       Binary Stdio Pipe | PTFR Frames (Stdout) / Button Mask (Stdin)
                                         v
+---------------------------------------------------------------------------------+
|               picoTrackerAdvance.exe (Native Windows Backend)                   |
|                                                                                 |
|  - Headless/Offscreen Video Driver (SDL_HINT_VIDEODRIVER="dummy")               |
|  - Native SDL2 Audio Engine -> WASAPI/DirectSound Endpoint (Zero Latency)       |
|  - DesktopBridge: Streams dirty 720x720 RGBA frames ("PTFR" magic)             |
|  - Input Thread: Dispatches 16-bit button masks directly to AppWindow event bus |
+---------------------------------------------------------------------------------+
```

---

## Controller & Input Mapping

### ROG Ally / XInput Gamepad

| Handheld Control | picoTracker Advance Function | Hardware Switch Equivalent |
|---|---|---|
| **D-Pad Up / Down / Left / Right** | Cursor Movement | SW4, SW2, SW1, SW3 |
| **Left Analog Stick** | Analog Cursor Movement (with 0.45 deadzone) | Directional Matrix |
| **A Button** | **A** (Value increment, audition, enter) | SW7 |
| **B Button** | **B** (Value decrement, edit, back) | SW6 |
| **X Button** | **VIEW** (Song / Chain / Phrase view toggle) | SELECT |
| **Y Button** | **A** (Alternate ergonomic thumb button) | SW7 |
| **LB (Left Bumper) / LT (Left Trigger)** | **NAV** (L modifier / navigation) | SW5 |
| **RB (Right Bumper) / RT (Right Trigger)** | **OPT** (R modifier / option menu) | SW8 |
| **View / Back Button** | **VIEW** (Select) | SELECT |
| **Menu / Start Button** | **PLAY / STOP** | SW9 |
| **Xbox / Guide Button** | **POWER** | POWER |

### Touchscreen Controls (Lateral Wings)

The left and right wings are proportioned to fit naturally under the user's thumbs on a 7-inch 16:9 handheld display:
- **Left Wing**: Directional Pad (▲ ◀ ▶ ▼), NAV (L-Shoulder), VIEW (Select).
- **Right Wing**: A (Enter/Value), B (Edit/Back), OPT (R-Shoulder), PLAY (Start).
- **Quick Combos**:
  - `DELETE` (`NAV + B`): Single-tap deletion of rows/notes.
  - `LATCH NAV`: Locks the NAV modifier on for one-handed navigation.
- **Pointer Capture**: Supports dragging fingers across buttons without losing touch state.

### Keyboard Shortcuts

- **Arrows**: Cursor Movement
- **Z / C / Enter**: A button
- **X / V / Backspace / Esc**: B button
- **Left Shift / Left Ctrl / Q / Tab**: NAV (L)
- **Right Shift / Right Ctrl / Alt / E**: OPT (R)
- **Space**: PLAY / STOP
- **Delete / Backquote (\`)**: VIEW
- **F11**: Fullscreen Toggle

---

## Features

1. **Aspect Ratio Modes**:
   - `1:1 SQUARE`: Native 720×720 square display (authentic picoTracker Advance aspect ratio).
   - `16:9 WIDE`: Widescreen stretch mode filling handheld display panels.
   - `4:3 STRETCH`: Classic retro CRT / game console aspect ratio.
2. **Layout Toggle**:
   - `CHASSIS`: Displays full handheld lateral wings with touch controls.
   - `SCREEN ONLY`: Collapses lateral wings for pure screen tracker viewing.
3. **Low-Latency Offscreen Audio**:
   - SDL2 audio renders natively on Windows directly to the primary output device, eliminating WebAudio IPC buffer jitter.
4. **Standalone Portability**:
   - Self-contained executable or folder with zero external dependencies.

---

## Running & Building

### Run in Development
```powershell
npm start
```

### Build Standalone Portable Executable
```powershell
npm run dist
```
The output will be generated in `dist/`:
- `dist/picoTrackerAdvance-ROGAlly.exe` (Single-file portable executable)
- `dist/win-unpacked/` (Unpacked standalone folder)
