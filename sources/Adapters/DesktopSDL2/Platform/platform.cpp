/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * platform.cpp - Desktop Platform implementation for picoTracker Advance
 */

#include "platform.h"
#include <SDL2/SDL.h>
#include <cstdlib>

class DesktopMutex : public SysMutex {
public:
  DesktopMutex() { mutex_ = SDL_CreateMutex(); }
  virtual ~DesktopMutex() {
    if (mutex_) {
      SDL_DestroyMutex(mutex_);
      mutex_ = nullptr;
    }
  }
  virtual bool Lock() override {
    return mutex_ ? (SDL_LockMutex(mutex_) == 0) : false;
  }
  virtual void Unlock() override {
    if (mutex_) {
      SDL_UnlockMutex(mutex_);
    }
  }

private:
  SDL_mutex *mutex_;
};

void platform_init() {}

void platform_reboot() { exit(0); }

void platform_bootloader() { exit(0); }

SysMutex *platform_mutex() {
  static DesktopMutex s_mutex;
  return &s_mutex;
}

uint32_t millis(void) { return (uint32_t)SDL_GetTicks(); }

uint32_t micros(void) {
  static uint64_t freq = SDL_GetPerformanceFrequency();
  return (uint32_t)((SDL_GetPerformanceCounter() * 1000000ULL) / freq);
}

uint32_t platform_get_rand() { return (uint32_t)rand(); }

void platform_brightness(uint8_t value) {}

int16_t battery_health() { return 100; }

#include "Adapters/adv/usb/usb_utils.h"

extern "C" {
bool usbHostModeActive(void) { return false; }
UsbHostControlSurfaceType usbHostControlSurfaceType(void) {
  return USB_HOST_CONTROL_SURFACE_NONE;
}
void sendUSBMidiMessage(const uint8_t *midicmd, uint8_t len) {}
void pumpUSBMidiTxQueue(void) {}
void handleUSBInterrupts() {}
void resetUSBMidiSoftwareState(void) {}
bool usbHostMidiMounted(void) { return false; }
bool usbHostDeviceEnumerated(void) { return false; }
bool usbHostEnumerationTimedOut(void) { return false; }
bool usbHostStartupFailed(void) { return false; }
}
