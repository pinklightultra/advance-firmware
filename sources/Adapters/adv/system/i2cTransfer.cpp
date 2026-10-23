/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#include "i2cTransfer.h"
#include "System/Console/Trace.h"

static HAL_StatusTypeDef finishTransfer(I2C_HandleTypeDef *hi2c,
                                        uint16_t address, const char *operation,
                                        HAL_StatusTypeDef status) {
  if (status == HAL_OK) {
    return status;
  }

  const uint32_t error = hi2c->ErrorCode;
  const uint32_t before = hi2c->Instance->ISR;
  bool stopIssued = false;
  // HAL_BUSY may mean another caller owns a healthy transfer. Never abort it.
  if (status != HAL_BUSY && hi2c->State == HAL_I2C_STATE_READY &&
      hi2c->Lock == HAL_UNLOCKED) {
    hi2c->Lock = HAL_LOCKED;
    if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BUSY) != RESET) {
      stopIssued = true;
      SET_BIT(hi2c->Instance->CR2, I2C_CR2_STOP);
      // Bound recovery if a device keeps the bus low. Do not reset the bus,
      // retry the transaction, or hide the original error from the caller.
      const uint32_t start = HAL_GetTick();
      while (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BUSY) != RESET &&
             (HAL_GetTick() - start) < 2U) {
      }
      if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BUSY) == RESET) {
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);
      }
    }
    hi2c->Lock = HAL_UNLOCKED;
  }
  const uint32_t after = hi2c->Instance->ISR;
  Trace::Error(
      "I2C %s addr=0x%02x status=%u error=0x%08lx STOP=%u "
      "ISR=0x%08lx->0x%08lx",
      operation, address >> 1, static_cast<unsigned>(status),
      static_cast<unsigned long>(error), static_cast<unsigned>(stopIssued),
      static_cast<unsigned long>(before), static_cast<unsigned long>(after));
  return status;
}

HAL_StatusTypeDef AdvI2C_Master_Transmit(I2C_HandleTypeDef *hi2c,
                                         uint16_t address, uint8_t *data,
                                         uint16_t size, uint32_t timeout) {
  const HAL_StatusTypeDef status =
      HAL_I2C_Master_Transmit(hi2c, address, data, size, timeout);
  return finishTransfer(hi2c, address, "Master_Transmit", status);
}

HAL_StatusTypeDef AdvI2C_Master_Receive(I2C_HandleTypeDef *hi2c,
                                        uint16_t address, uint8_t *data,
                                        uint16_t size, uint32_t timeout) {
  const HAL_StatusTypeDef status =
      HAL_I2C_Master_Receive(hi2c, address, data, size, timeout);
  return finishTransfer(hi2c, address, "Master_Receive", status);
}

HAL_StatusTypeDef AdvI2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t address,
                                   uint16_t reg, uint16_t regSize,
                                   uint8_t *data, uint16_t size,
                                   uint32_t timeout) {
  const HAL_StatusTypeDef status =
      HAL_I2C_Mem_Write(hi2c, address, reg, regSize, data, size, timeout);
  return finishTransfer(hi2c, address, "Mem_Write", status);
}

HAL_StatusTypeDef AdvI2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t address,
                                  uint16_t reg, uint16_t regSize, uint8_t *data,
                                  uint16_t size, uint32_t timeout) {
  const HAL_StatusTypeDef status =
      HAL_I2C_Mem_Read(hi2c, address, reg, regSize, data, size, timeout);
  return finishTransfer(hi2c, address, "Mem_Read", status);
}
