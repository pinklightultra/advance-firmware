/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#ifndef ADV_I2C_TRANSFER_H
#define ADV_I2C_TRANSFER_H

#include "i2c.h"

// Shared transfer timeout for the codec, battery gauge, charger and USB role
// IC.
#define I2C_TRANSFER_TIMEOUT_MS 25U

#ifdef __cplusplus
extern "C" {
#endif

// Blocking transfers preserve HAL status and attempt STOP recovery on failure.
HAL_StatusTypeDef AdvI2C_Master_Transmit(I2C_HandleTypeDef *hi2c,
                                         uint16_t address, uint8_t *data,
                                         uint16_t size, uint32_t timeout);
HAL_StatusTypeDef AdvI2C_Master_Receive(I2C_HandleTypeDef *hi2c,
                                        uint16_t address, uint8_t *data,
                                        uint16_t size, uint32_t timeout);
HAL_StatusTypeDef AdvI2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t address,
                                   uint16_t reg, uint16_t regSize,
                                   uint8_t *data, uint16_t size,
                                   uint32_t timeout);
HAL_StatusTypeDef AdvI2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t address,
                                  uint16_t reg, uint16_t regSize, uint8_t *data,
                                  uint16_t size, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif // ADV_I2C_TRANSFER_H
