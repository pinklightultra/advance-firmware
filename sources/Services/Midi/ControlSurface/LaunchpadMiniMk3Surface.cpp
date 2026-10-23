/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "ControlSurface/LaunchpadMiniMk3Surface.h"
#include "Application/AppWindow.h"
#include "Application/Model/Song.h"
#include "Application/Player/Player.h"
#include "ControlSurface/ColorConversion.h"
#include "Services/Midi/MidiMessage.h"
#include "Services/Midi/MidiService.h"

namespace {
constexpr uint16_t kProfileOrderLaunchpadRefresh = 81;
constexpr int kLaunchpadGridMinNote = 11;
constexpr int kLaunchpadGridMaxNote = 88;
constexpr int kLaunchpadTopRowMinCc = 91;
constexpr int kLaunchpadTopRowMaxCc = 98;
constexpr int kLaunchpadRightColumnMinCc = 19;
constexpr int kLaunchpadRightColumnMaxCc = 89;
constexpr int kLaunchpadUpCc = 91;
constexpr int kLaunchpadDownCc = 92;
constexpr int kLaunchpadLeftCc = 93;
constexpr int kLaunchpadRightCc = 94;
constexpr int kLaunchpadUserCc = 98;

constexpr uint8_t kLedModeStatic = 0;
constexpr uint8_t kLedModePulse = 2;

constexpr uint8_t kColorOff = 0;
constexpr uint8_t kColorDimBlue = 37;
constexpr uint8_t kColorDimRed = 5;
constexpr uint8_t kColorYellow = 13;
constexpr uint8_t kColorGreen = 21;
constexpr uint8_t kColorDimGreen = 17;
constexpr uint8_t kColorAmber = 9;
constexpr uint8_t kColorCyan = 33;

constexpr uint8_t kCacheInvalid = 0xFF;
constexpr uint8_t kLedModeDirectRgbCache = 0xFE;
constexpr uint32_t kRgbCacheInvalid = 0xFFFFFFFFu;

} // namespace

LaunchpadMiniMk3Surface::LaunchpadMiniMk3Surface()
    : ControlSurfaceDevice(kLaunchpadButtonCount),
      programmerModeEnabled_(false), baseGridColorsCached_(false), emptyRgb_(0),
      noteOffRgb_(0), firstPatternRgb_(0), patternRgb_(0) {
  resetLedCache();
}

LaunchpadMiniMk3Surface::~LaunchpadMiniMk3Surface() { Stop(); }

bool LaunchpadMiniMk3Surface::Init(MidiService &midiService) {
  ControlSurfaceDevice::Init(midiService);
  programmerModeEnabled_ = false;
  baseGridColorsCached_ = false;
  resetLedCache();
  return true;
}

bool LaunchpadMiniMk3Surface::OnMidiMessage(MidiMessage &message) {
  if (!isRecognizedLaunchpadControl(message)) {
    return false;
  }

  if (!IsEnabled()) {
    return false;
  }

  (void)EnsureRuntimeBindings();

  int channel = 0;
  int row = 0;
  if (decodeGridPad(message.data1_, channel, row)) {
    if (IsLiveModeActive() && isPress(message)) {
      HandleGridPadPress(channel, row);
      message.Consume();
      return true;
    }
    return false;
  }

  if (message.GetType() != MidiMessage::MIDI_CONTROL_CHANGE) {
    return false;
  }

  if (decodeTopRowButton(message.data1_, channel)) {
    if (!IsLiveModeActive()) {
      return false;
    }

    if (isPress(message)) {
      if (IsSoloMuteHeld()) {
        HandleMuteActionPress(channel);
        message.Consume();
        return true;
      }

      const int cc = kLaunchpadTopRowMinCc + channel;
      if (cc == kLaunchpadUpCc) {
        HandleSongWindowFineUpPress();
      } else if (cc == kLaunchpadDownCc) {
        HandleSongWindowFineDownPress();
      } else if (cc == kLaunchpadLeftCc) {
        HandleSongWindowPageUpPress();
      } else if (cc == kLaunchpadRightCc) {
        HandleSongWindowPageDownPress();
      } else if (channel == (kLaunchpadButtonCount - 1)) {
        HandleTransportPress();
      } else {
        return false;
      }

      message.Consume();
      return true;
    }

    if (isRelease(message)) {
      bool handled = false;
      if (IsMuteActionHeld(channel)) {
        HandleMuteActionRelease(channel);
        handled = true;
      }

      if (handled) {
        message.Consume();
        return true;
      }

      return false;
    }

    return false;
  }

  int index = 0;
  if (decodeRightColumnButton(message.data1_, index)) {
    if (!IsLiveModeActive()) {
      return false;
    }

    if (isPress(message)) {
      if (index == (kLaunchpadButtonCount - 1)) {
        HandleMuteModifierPress();
      } else {
        HandleSceneLaunchPress(index);
      }
      message.Consume();
      return true;
    }

    if (isRelease(message) && (index == (kLaunchpadButtonCount - 1)) &&
        IsSoloMuteHeld()) {
      HandleMuteModifierRelease();
      message.Consume();
      return true;
    }

    return false;
  }

  return false;
}

void LaunchpadMiniMk3Surface::RefreshLeds() {
  if (!IsStarted() || !IsEnabled() || !AreLedsDirty() ||
      (GetMidiService() == nullptr)) {
    return;
  }

  ControlSurfaceState state;
  GetPlayer()->FillControlSurfaceState(state, GetSongWindowOffset(),
                                       kLaunchpadButtonCount);

  if (!state.isLiveMode) {
    ResetSongWindowControlState();
    if (programmerModeEnabled_) {
      clearAllLeds(true);
      setProgrammerMode(false);
    }
    ClearLedsDirty();
    return;
  }

  if (!programmerModeEnabled_) {
    setProgrammerMode(true);
  }

  cacheBaseGridColors();

  uint16_t pos = 0;

  for (int row = 0; row < kLaunchpadButtonCount; ++row) {
    for (int channel = 0; channel < kLaunchpadButtonCount; ++channel) {
      uint8_t color = kColorOff;
      uint8_t mode = kLedModeStatic;
      uint32_t baseRgb = 0;
      bool useRgb = false;

      switch (state.cells[channel][row]) {
      case ControlSurfaceState::Cell::Playing:
        color = kColorGreen;
        mode = kLedModePulse;
        break;
      case ControlSurfaceState::Cell::Queued:
        color = kColorYellow;
        break;
      case ControlSurfaceState::Cell::Muted:
        color = kColorDimRed;
        break;
      case ControlSurfaceState::Cell::NoteOff:
        baseRgb = noteOffRgb_;
        useRgb = true;
        break;
      case ControlSurfaceState::Cell::FirstPattern:
        baseRgb = firstPatternRgb_;
        useRgb = true;
        break;
      case ControlSurfaceState::Cell::Populated:
        baseRgb = patternRgb_;
        useRgb = true;
        break;
      case ControlSurfaceState::Cell::Empty:
      default:
        baseRgb = emptyRgb_;
        useRgb = true;
        break;
      }

      if (useRgb) {
        setGridLedRgb(channel, row, baseRgb);
      } else {
        setGridLed(channel, row, color, mode, &pos);
      }
    }
  }

  for (int i = 0; i < kLaunchpadButtonCount; ++i) {
    uint8_t color = kColorOff;
    uint8_t mode = kLedModeStatic;

    if (i == (kLaunchpadButtonCount - 1)) {
      color = kColorAmber;
      if (IsSoloMuteHeld()) {
        mode = kLedModePulse;
      }
      setRightColumnLed(i, color, mode, &pos);
      continue;
    }

    const auto rowState = GetRowState(state, kLaunchpadButtonCount, i);

    if (rowState.playing) {
      color = kColorGreen;
      mode = kLedModePulse;
    } else if (rowState.queued) {
      color = kColorYellow;
    } else if (rowState.playable) {
      color = kColorCyan;
    }

    setRightColumnLed(i, color, mode, &pos);
  }

  const bool liveRunning = state.isLiveMode && state.isRunning;
  for (int channel = 0; channel < kLaunchpadButtonCount; ++channel) {
    const int cc = kLaunchpadTopRowMinCc + channel;
    uint8_t color = kColorOff;
    uint8_t mode = kLedModeStatic;

    if (IsSoloMuteHeld()) {
      // We don't have per-channel mute state in the new GridState yet,
      // but since the original implementation queried Player directly here,
      // we'll keep the direct query for this specific Launchpad-only feature
      // for now to maintain exact behavior.
      color =
          GetPlayer()->IsChannelMuted(channel) ? kColorDimRed : kColorDimGreen;
    } else if (cc == kLaunchpadUserCc) {
      color = liveRunning ? kColorGreen : kColorDimGreen;
      mode = liveRunning ? kLedModePulse : kLedModeStatic;
    }

    setTopRowLed(cc, color, mode, &pos);
  }

  if (pos > 0) {
    sendBufferedMidiMessages(pos);
  }

  ClearLedsDirty();
}

bool LaunchpadMiniMk3Surface::isRecognizedLaunchpadControl(
    const MidiMessage &message) const {
  switch (message.status_ & 0xF0) {
  case MidiMessage::MIDI_NOTE_ON:
  case MidiMessage::MIDI_NOTE_OFF: {
    int channel = 0;
    int row = 0;
    return decodeGridPad(message.data1_, channel, row);
  }
  case MidiMessage::MIDI_CONTROL_CHANGE: {
    int index = 0;
    return decodeRightColumnButton(message.data1_, index) ||
           isTopRowButton(message.data1_);
  }
  default:
    return false;
  }
}

bool LaunchpadMiniMk3Surface::decodeGridPad(int note, int &channel,
                                            int &row) const {
  if ((note < kLaunchpadGridMinNote) || (note > kLaunchpadGridMaxNote)) {
    return false;
  }

  const int ones = note % 10;
  const int tens = note / 10;
  if ((ones < 1) || (ones > 8) || (tens < 1) || (tens > 8)) {
    return false;
  }

  channel = ones - 1;
  row = 8 - tens;
  return true;
}

bool LaunchpadMiniMk3Surface::decodeRightColumnButton(int cc,
                                                      int &index) const {
  if ((cc < kLaunchpadRightColumnMinCc) || (cc > kLaunchpadRightColumnMaxCc)) {
    return false;
  }

  const int ones = cc % 10;
  const int tens = cc / 10;
  if ((ones != 9) || (tens < 1) || (tens > 8)) {
    return false;
  }

  index = 8 - tens;
  return true;
}

bool LaunchpadMiniMk3Surface::isTopRowButton(int cc) const {
  return (cc >= kLaunchpadTopRowMinCc) && (cc <= kLaunchpadTopRowMaxCc);
}

bool LaunchpadMiniMk3Surface::decodeTopRowButton(int cc, int &channel) const {
  if (!isTopRowButton(cc)) {
    return false;
  }

  channel = cc - kLaunchpadTopRowMinCc;
  return (channel >= 0) && (channel < kLaunchpadButtonCount);
}

void LaunchpadMiniMk3Surface::OnSurfaceStopped() {
  clearAllLeds(true);
  setProgrammerMode(false);
  programmerModeEnabled_ = false;
  baseGridColorsCached_ = false;
}

void LaunchpadMiniMk3Surface::OnViewDataChanged(I_ObservableData *data) {
  (void)data;
  baseGridColorsCached_ = false;
  cacheBaseGridColors();
}

void LaunchpadMiniMk3Surface::cacheBaseGridColors() {
  if (baseGridColorsCached_) {
    return;
  }

  emptyRgb_ = 0;
  noteOffRgb_ = packGuiColor(AppWindow::GetColor(CD_ACCENT));
  firstPatternRgb_ = packGuiColor(AppWindow::GetColor(CD_HILITE1));
  patternRgb_ = packGuiColor(AppWindow::GetColor(CD_NORMAL));
  baseGridColorsCached_ = true;
}

void LaunchpadMiniMk3Surface::setProgrammerMode(bool enabled) {
  if ((GetMidiService() == nullptr) || (programmerModeEnabled_ == enabled)) {
    return;
  }

  const uint8_t modeSysex[] = {
      0xF0, 0x00, 0x20, 0x29,
      0x02, 0x0D, 0x0E, static_cast<uint8_t>(enabled ? 1 : 0),
      0xF7};
  GetMidiService()->SendControlSurfaceRawImmediate(modeSysex,
                                                   sizeof(modeSysex));
  if (enabled) {
    const uint8_t layoutSysex[] = {0xF0, 0x00, 0x20, 0x29, 0x02,
                                   0x0D, 0x00, 0x7F, 0xF7};
    GetMidiService()->SendControlSurfaceRawImmediate(layoutSysex,
                                                     sizeof(layoutSysex));
  }
  programmerModeEnabled_ = enabled;
  resetLedCache();
}

void LaunchpadMiniMk3Surface::clearAllLeds(bool force) {
  if (GetMidiService() == nullptr) {
    return;
  }

  if (force) {
    resetLedCache();
  }

  uint16_t pos = 0;

  for (int row = 0; row < kLaunchpadButtonCount; ++row) {
    for (int channel = 0; channel < kLaunchpadButtonCount; ++channel) {
      setGridLed(channel, row, kColorOff, kLedModeStatic, &pos);
    }
  }

  for (int i = 0; i < kLaunchpadButtonCount; ++i) {
    setRightColumnLed(i, kColorOff, kLedModeStatic, &pos);
  }

  for (int cc = kLaunchpadTopRowMinCc; cc <= kLaunchpadTopRowMaxCc; ++cc) {
    setTopRowLed(cc, kColorOff, kLedModeStatic, &pos);
  }

  if (pos > 0) {
    sendBufferedMidiMessages(pos);
  }
}

void LaunchpadMiniMk3Surface::resetLedCache() {
  for (int row = 0; row < kLaunchpadButtonCount; ++row) {
    for (int channel = 0; channel < kLaunchpadButtonCount; ++channel) {
      gridLedColor_[channel][row] = kCacheInvalid;
      gridLedMode_[channel][row] = kCacheInvalid;
      gridLedRgb_[channel][row] = kRgbCacheInvalid;
    }
  }

  for (int i = 0; i < kLaunchpadButtonCount; ++i) {
    rightLedColor_[i] = kCacheInvalid;
    rightLedMode_[i] = kCacheInvalid;
    topLedColor_[i] = kCacheInvalid;
    topLedMode_[i] = kCacheInvalid;
  }
}

void LaunchpadMiniMk3Surface::sendPadLedNote(int note, uint8_t color,
                                             uint8_t lightingMode,
                                             uint16_t *pos) {
  if (GetMidiService() == nullptr) {
    return;
  }

  const uint8_t status =
      static_cast<uint8_t>(MidiMessage::MIDI_NOTE_ON + lightingMode);
  appendMidiMessage(*pos, status, static_cast<uint8_t>(note), color);
}

void LaunchpadMiniMk3Surface::sendPadLedRgb(int note, uint32_t rgb) {
  if (GetMidiService() == nullptr) {
    return;
  }

  const uint8_t sysex[] = {0xF0,
                           0x00,
                           0x20,
                           0x29,
                           0x02,
                           0x0D,
                           0x03,
                           0x03,
                           static_cast<uint8_t>(note),
                           static_cast<uint8_t>((rgb >> 16) & 0x7F),
                           static_cast<uint8_t>((rgb >> 8) & 0x7F),
                           static_cast<uint8_t>(rgb & 0x7F),
                           0xF7};
  GetMidiService()->SendControlSurfaceRawImmediate(sysex, sizeof(sysex));
}

void LaunchpadMiniMk3Surface::sendButtonLedCc(int cc, uint8_t color,
                                              uint8_t lightingMode,
                                              uint16_t *pos) {
  if (GetMidiService() == nullptr) {
    return;
  }

  const uint8_t status =
      static_cast<uint8_t>(MidiMessage::MIDI_CONTROL_CHANGE + lightingMode);
  appendMidiMessage(*pos, status, static_cast<uint8_t>(cc), color);
}

void LaunchpadMiniMk3Surface::setGridLed(int channel, int row, uint8_t color,
                                         uint8_t lightingMode, uint16_t *pos) {
  if ((channel < 0) || (channel >= kLaunchpadButtonCount) || (row < 0) ||
      (row >= kLaunchpadButtonCount)) {
    return;
  }

  if ((gridLedColor_[channel][row] == color) &&
      (gridLedMode_[channel][row] == lightingMode)) {
    return;
  }

  gridLedColor_[channel][row] = color;
  gridLedMode_[channel][row] = lightingMode;
  gridLedRgb_[channel][row] = kRgbCacheInvalid;
  sendPadLedNote(gridNoteFor(channel, row), color, lightingMode, pos);
}

void LaunchpadMiniMk3Surface::setGridLedRgb(int channel, int row,
                                            uint32_t rgb) {
  if ((channel < 0) || (channel >= kLaunchpadButtonCount) || (row < 0) ||
      (row >= kLaunchpadButtonCount)) {
    return;
  }

  if ((gridLedMode_[channel][row] == kLedModeDirectRgbCache) &&
      (gridLedRgb_[channel][row] == rgb)) {
    return;
  }

  gridLedColor_[channel][row] = kCacheInvalid;
  gridLedMode_[channel][row] = kLedModeDirectRgbCache;
  gridLedRgb_[channel][row] = rgb;
  sendPadLedRgb(gridNoteFor(channel, row), rgb);
}

void LaunchpadMiniMk3Surface::setRightColumnLed(int index, uint8_t color,
                                                uint8_t lightingMode,
                                                uint16_t *pos) {
  if ((index < 0) || (index >= kLaunchpadButtonCount)) {
    return;
  }

  if ((rightLedColor_[index] == color) &&
      (rightLedMode_[index] == lightingMode)) {
    return;
  }

  rightLedColor_[index] = color;
  rightLedMode_[index] = lightingMode;
  sendButtonLedCc(rightColumnCcFor(index), color, lightingMode, pos);
}

void LaunchpadMiniMk3Surface::setTopRowLed(int cc, uint8_t color,
                                           uint8_t lightingMode,
                                           uint16_t *pos) {
  const int index = topRowIndexForCc(cc);
  if (index < 0) {
    return;
  }

  if ((topLedColor_[index] == color) && (topLedMode_[index] == lightingMode)) {
    return;
  }

  topLedColor_[index] = color;
  topLedMode_[index] = lightingMode;
  sendButtonLedCc(cc, color, lightingMode, pos);
}

int LaunchpadMiniMk3Surface::gridNoteFor(int channel, int row) const {
  return (kLaunchpadButtonCount - row) * 10 + (channel + 1);
}

int LaunchpadMiniMk3Surface::rightColumnCcFor(int index) const {
  return (kLaunchpadButtonCount - index) * 10 + 9;
}

int LaunchpadMiniMk3Surface::topRowIndexForCc(int cc) const {
  if ((cc < kLaunchpadTopRowMinCc) || (cc > kLaunchpadTopRowMaxCc)) {
    return -1;
  }
  return cc - kLaunchpadTopRowMinCc;
}
