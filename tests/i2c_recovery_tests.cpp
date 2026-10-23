/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#include "doctest/doctest.h"
// Exercise the production wrappers with simulated HAL transfers/registers.
#include "../sources/Adapters/adv/system/i2cTransfer.cpp"

TEST_CASE(
    "I2C failure recovery preserves transfer results and bounds waiting") {
  I2C_TypeDef regs;
  I2C_HandleTypeDef handle{&regs};
  uint8_t data = 0;
  i2cTest::tick = 0;
  i2cTest::calls = 0;
  i2cTest::cleared = 0;
  i2cTest::releaseOnStop = true;
  i2cTest::result = HAL_ERROR;
  regs.CR2 = 0x100d6;
  regs.ISR = I2C_FLAG_BUSY;
  handle.ErrorCode = 0x20;
  bool expectStop = true;

  SUBCASE("Successful transfer does not touch the peripheral") {
    i2cTest::result = HAL_OK;
    expectStop = false;
  }
  SUBCASE("Failed idle bus requires no STOP") {
    regs.ISR = 0;
    expectStop = false;
  }
  SUBCASE("HAL_BUSY does not abort another transfer") {
    i2cTest::result = HAL_BUSY;
    expectStop = false;
  }
  SUBCASE("Active handle is left alone") {
    handle.State = HAL_I2C_STATE_BUSY_RX;
    expectStop = false;
  }
  SUBCASE("Locked handle is left alone") {
    handle.Lock = HAL_LOCKED;
    expectStop = false;
  }
  SUBCASE("Timeout also triggers recovery") { i2cTest::result = HAL_TIMEOUT; }
  SUBCASE("Stuck bus recovery terminates without clearing STOPF") {
    i2cTest::releaseOnStop = false;
  }
  const auto oldLock = handle.Lock;
  const auto status = AdvI2C_Mem_Read(&handle, 0xd6, 8, 1, &data, 1, 50);
  CHECK(status == i2cTest::result);
  CHECK(i2cTest::calls == 1); // Recovery never retries the failed operation.
  CHECK(handle.ErrorCode == 0x20);
  CHECK(handle.Lock == oldLock);
  CHECK(regs.CR2 == (0x100d6U | (expectStop ? I2C_CR2_STOP : 0U)));
  CHECK(i2cTest::tick <= 4);
  CHECK(i2cTest::cleared ==
        (expectStop && i2cTest::releaseOnStop ? I2C_FLAG_STOPF : 0U));
}

TEST_CASE("All blocking I2C wrappers recover errors without retries") {
  for (int operation = 0; operation < 4; ++operation) {
    I2C_TypeDef regs;
    regs.ISR = I2C_FLAG_BUSY;
    I2C_HandleTypeDef handle{&regs};
    uint8_t data = 0;
    i2cTest::result = HAL_ERROR;
    i2cTest::releaseOnStop = true;
    i2cTest::calls = 0;
    HAL_StatusTypeDef status;
    switch (operation) {
    case 0:
      status = AdvI2C_Master_Transmit(&handle, 0xaa, &data, 1, 50);
      break;
    case 1:
      status = AdvI2C_Master_Receive(&handle, 0xaa, &data, 1, 50);
      break;
    case 2:
      status = AdvI2C_Mem_Write(&handle, 0xaa, 8, 1, &data, 1, 50);
      break;
    default:
      status = AdvI2C_Mem_Read(&handle, 0xaa, 8, 1, &data, 1, 50);
      break;
    }
    CHECK(status == HAL_ERROR);
    CHECK(i2cTest::calls == 1);
    CHECK((regs.CR2 & I2C_CR2_STOP) != 0);
  }
}
