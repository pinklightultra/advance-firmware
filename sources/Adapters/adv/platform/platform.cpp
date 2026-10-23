/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "platform.h"
#include "Adapters/adv/mutex/advMutex.h"
#include "Adapters/adv/system/BatteryGauge.h"
#include "System/Console/Trace.h"
#include "tim.h"
#include <System/Console/nanoprintf.h>

namespace {

// This must not use the stack after CONTROL selects MSP because the caller is
// running on the FreeRTOS process stack.
[[noreturn]] __attribute__((naked)) void
jumpToBootloader(uint32_t appStack, uint32_t appResetHandler) {
  __asm volatile("msr msp, r0\n"
                 "movs r2, #0\n"
                 "msr control, r2\n"
                 "isb\n"
                 "msr basepri, r2\n"
                 "msr faultmask, r2\n"
                 "cpsie i\n"
                 "dsb\n"
                 "isb\n"
                 "bx r1\n");
}

} // namespace

void platform_reboot() { NVIC_SystemReset(); };

void platform_bootloader() {

  // Disable external interrupts first since they can fire during this process
  __disable_irq();

  uint32_t appStack = *(volatile uint32_t *)BOOTLOADER_ADDR;
  uint32_t appResetHandler = *(volatile uint32_t *)(BOOTLOADER_ADDR + 4);

  // Disable and clear every external interrupt. An interrupt using the new
  // vector table must not run before the bootloader initializes it.
  for (size_t index = 0; index < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]);
       ++index) {
    NVIC->ICER[index] = UINT32_MAX;
    NVIC->ICPR[index] = UINT32_MAX;
  }

  // Disable SysTick (this may not be needed, not using it)
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

  // We disable TIM8 because that's what we are using as systick
  // 1. Disable TIM8 IRQ at peripheral level
  TIM8->DIER = 0;

  // 2. Stop counter
  TIM8->CR1 &= ~TIM_CR1_CEN;

  // 3. Clear all flags
  TIM8->SR = 0;

  // Disable caches
  // D-cache must be cleaned before HAL_DeInit resets the external memory bus.
  SCB_DisableDCache();
  SCB_DisableICache();

  HAL_DeInit();
  HAL_RCC_DeInit(); // Reset clock configuration

  // Disable MPU
  HAL_MPU_Disable();

  // Memory barrier
  __DSB();
  __ISB();

  // Set vector table offset
  SCB->VTOR = BOOTLOADER_ADDR;

  // Set MSP
  // Jump to application
  jumpToBootloader(appStack, appResetHandler);
};

SysMutex *platform_mutex() {
  static advMutex mutex;
  return &mutex;
};

uint32_t millis(void) { return __HAL_TIM_GET_COUNTER(&htim2) / 1000; }
uint32_t micros(void) { return __HAL_TIM_GET_COUNTER(&htim2); }

void platform_brightness(uint8_t value) {
  uint8_t min_brightness = 20;

  // Calculate effective range: from min_percent up to 100%
  float effective_range = 100.0f - min_brightness;

  // Normalize value (0–255) to a 0.0–1.0 range
  float normalized = (float)value / 255.0f;

  // Final percentage to set
  float pwm_percent = min_brightness + normalized * effective_range;

  // Convert to timer pulse (assuming Period = 999 for 100%)
  uint32_t pulse = (uint32_t)((pwm_percent / 100.0f) *
                              (__HAL_TIM_GET_AUTORELOAD(&htim13) + 1));

  __HAL_TIM_SET_COMPARE(&htim13, TIM_CHANNEL_1, pulse);
}

// returns -1 on error, -2 for "calculating" status
int16_t battery_health() {
  auto soh_type = getBatteryStateOfHealthType();
  if (soh_type == SOH_READY) {
    return getBatteryStateOfHealth();
  } else {
    return -2;
  }
};
