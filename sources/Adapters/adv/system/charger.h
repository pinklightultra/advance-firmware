/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2025 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */
#ifndef _ADV_CHARGER_H_
#define _ADV_CHARGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t chargerHostPowerReg01Value(bool enabled) {
  return enabled ? 0x2Bu : 0x1Bu;
}

static inline bool chargerOtgConfigFromReg01(uint8_t reg01) {
  return (reg01 & 0x20u) != 0u;
}

static inline bool chargerChargeConfigFromReg01(uint8_t reg01) {
  return (reg01 & 0x10u) != 0u;
}

static inline uint8_t chargerSystemMinimumFromReg01(uint8_t reg01) {
  return (reg01 >> 1u) & 0x07u;
}

static inline bool chargerOtgBatteryLow2500FromReg01(uint8_t reg01) {
  return (reg01 & 0x01u) != 0u;
}

static inline bool chargerInputHighImpedanceFromReg00(uint8_t reg00) {
  return (reg00 & 0x80u) != 0u;
}

static inline uint16_t chargerInputCurrentLimitMaFromReg00(uint8_t reg00) {
  return (uint16_t)(((reg00 & 0x1Fu) + 1u) * 100u);
}

static inline bool chargerBoostLimitHighFromReg02(uint8_t reg02) {
  return (reg02 & 0x80u) != 0u;
}

static inline bool chargerBatfetDisabledFromReg07(uint8_t reg07) {
  return (reg07 & 0x20u) != 0u;
}

static inline bool chargerVbusGoodFromReg0A(uint8_t reg0a) {
  return (reg0a & 0x80u) != 0u;
}

static inline bool chargerAcOverVoltageFromReg0A(uint8_t reg0a) {
  return (reg0a & 0x04u) != 0u;
}

static inline uint8_t chargerVbusStatusFromReg08(uint8_t reg08) {
  return (reg08 >> 5u) & 0x07u;
}

static inline bool chargerVbusStatusIsOtg(uint8_t reg08) {
  return chargerVbusStatusFromReg08(reg08) == 0x07u;
}

static inline uint8_t chargerChargeStatusFromReg08(uint8_t reg08) {
  return (reg08 >> 3u) & 0x03u;
}

static inline bool chargerPowerGoodFromReg08(uint8_t reg08) {
  return (reg08 & 0x04u) != 0u;
}

static inline bool chargerThermalRegulationFromReg08(uint8_t reg08) {
  return (reg08 & 0x02u) != 0u;
}

static inline bool chargerVsysMinRegulationFromReg08(uint8_t reg08) {
  return (reg08 & 0x01u) != 0u;
}

static inline bool chargerWatchdogFaultFromReg09(uint8_t reg09) {
  return (reg09 & 0x80u) != 0u;
}

static inline bool chargerBoostFaultFromReg09(uint8_t reg09) {
  return (reg09 & 0x40u) != 0u;
}

static inline uint8_t chargerChargeFaultFromReg09(uint8_t reg09) {
  return (reg09 >> 4u) & 0x03u;
}

static inline bool chargerBatteryFaultFromReg09(uint8_t reg09) {
  return (reg09 & 0x08u) != 0u;
}

static inline uint8_t chargerNtcFaultFromReg09(uint8_t reg09) {
  return reg09 & 0x07u;
}

bool startCharging(void);
bool stopCharging(void);
bool setHostPowerEnabled(bool enabled);
void logHostPowerDiagnostics(const char *reason);
void powerOff();
void configureCharger(void);

typedef enum {
  NOT_CHARGING,
  PRE_CHARGE,
  FAST_CHARGE,
  CHARGE_DONE
} ChargingStatus;

typedef enum {
  VBUS_OK,
  VBUS_KO,
  CHARGE_END,
  CHARGE_FAULT,
  UNKNOWN,
} ChargerInt;

ChargingStatus getChargingStatus();
ChargerInt chargerIntReason();

#ifdef __cplusplus
}
#endif

#endif
