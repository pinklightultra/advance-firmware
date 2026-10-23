/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#pragma once

#include "ControlSurfaceDevice.h"
#include <stdint.h>

class MidiService;
struct MidiMessage;

class LaunchpadMiniMk3Surface : public ControlSurfaceDevice {
public:
  LaunchpadMiniMk3Surface();
  ~LaunchpadMiniMk3Surface() override;

  bool Init(MidiService &midiService) override;

  bool OnMidiMessage(MidiMessage &message) override;
  void RefreshLeds() override;

private:
  static constexpr int kLaunchpadButtonCount = 8;

  bool isRecognizedLaunchpadControl(const MidiMessage &message) const;
  bool decodeGridPad(int note, int &channel, int &row) const;
  bool decodeRightColumnButton(int cc, int &index) const;
  bool isTopRowButton(int cc) const;
  bool decodeTopRowButton(int cc, int &channel) const;
  void OnViewDataChanged(I_ObservableData *data) override;
  void OnSurfaceStopped() override;
  void cacheBaseGridColors();
  void setProgrammerMode(bool enabled);
  void clearAllLeds(bool force = false);
  void resetLedCache();
  void sendPadLedNote(int note, uint8_t color, uint8_t lightingMode,
                      uint16_t *pos = nullptr);
  void sendPadLedRgb(int note, uint32_t rgb);
  void sendButtonLedCc(int cc, uint8_t color, uint8_t lightingMode,
                       uint16_t *pos = nullptr);
  void setGridLed(int channel, int row, uint8_t color, uint8_t lightingMode,
                  uint16_t *pos = nullptr);
  void setGridLedRgb(int channel, int row, uint32_t rgb);
  void setRightColumnLed(int index, uint8_t color, uint8_t lightingMode,
                         uint16_t *pos = nullptr);
  void setTopRowLed(int cc, uint8_t color, uint8_t lightingMode,
                    uint16_t *pos = nullptr);
  int gridNoteFor(int channel, int row) const;
  int rightColumnCcFor(int index) const;
  int topRowIndexForCc(int cc) const;

private:
  bool programmerModeEnabled_;
  bool baseGridColorsCached_;
  uint32_t emptyRgb_;
  uint32_t noteOffRgb_;
  uint32_t firstPatternRgb_;
  uint32_t patternRgb_;
  uint8_t gridLedColor_[kLaunchpadButtonCount][kLaunchpadButtonCount];
  uint8_t gridLedMode_[kLaunchpadButtonCount][kLaunchpadButtonCount];
  uint32_t gridLedRgb_[kLaunchpadButtonCount][kLaunchpadButtonCount];
  uint8_t rightLedColor_[kLaunchpadButtonCount];
  uint8_t rightLedMode_[kLaunchpadButtonCount];
  uint8_t topLedColor_[kLaunchpadButtonCount];
  uint8_t topLedMode_[kLaunchpadButtonCount];
};
