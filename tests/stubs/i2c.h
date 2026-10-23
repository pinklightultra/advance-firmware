/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#pragma once
#include <cstdint>
enum HAL_StatusTypeDef { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT };
enum { HAL_UNLOCKED, HAL_LOCKED, HAL_I2C_STATE_READY, HAL_I2C_STATE_BUSY_RX };
constexpr uint32_t I2C_FLAG_BUSY = 0x8000;
constexpr uint32_t I2C_FLAG_STOPF = 0x20;
constexpr uint32_t I2C_CR2_STOP = 0x4000;
constexpr uint32_t RESET = 0;
struct I2C_TypeDef {
  uint32_t CR2 = 0;
  uint32_t ISR = 0;
};
struct I2C_HandleTypeDef {
  I2C_TypeDef *Instance;
  uint32_t ErrorCode = 0;
  uint32_t State = HAL_I2C_STATE_READY;
  uint32_t Lock = HAL_UNLOCKED;
};
namespace i2cTest {
inline uint32_t tick = 0;
inline bool releaseOnStop = true;
inline HAL_StatusTypeDef result = HAL_OK;
inline unsigned calls = 0;
inline uint32_t cleared = 0;
inline uint32_t flag(I2C_HandleTypeDef *h, uint32_t mask) {
  if (releaseOnStop && (h->Instance->CR2 & I2C_CR2_STOP)) {
    h->Instance->ISR &= ~I2C_FLAG_BUSY;
    h->Instance->ISR |= I2C_FLAG_STOPF;
  }
  return h->Instance->ISR & mask;
}
} // namespace i2cTest
#define SET_BIT(reg, bit) ((reg) |= (bit))
#define __HAL_I2C_GET_FLAG(h, mask) i2cTest::flag(h, mask)
#define __HAL_I2C_CLEAR_FLAG(h, flag) (i2cTest::cleared |= (flag))
inline uint32_t HAL_GetTick() { return i2cTest::tick++; }
inline HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *, uint16_t,
                                                 uint8_t *, uint16_t,
                                                 uint32_t) {
  ++i2cTest::calls;
  return i2cTest::result;
}
inline HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h,
                                                uint16_t a, uint8_t *d,
                                                uint16_t n, uint32_t t) {
  return HAL_I2C_Master_Transmit(h, a, d, n, t);
}
inline HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t a,
                                           uint16_t, uint16_t, uint8_t *d,
                                           uint16_t n, uint32_t t) {
  return HAL_I2C_Master_Transmit(h, a, d, n, t);
}
inline HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t a,
                                          uint16_t r, uint16_t s, uint8_t *d,
                                          uint16_t n, uint32_t t) {
  return HAL_I2C_Mem_Write(h, a, r, s, d, n, t);
}
