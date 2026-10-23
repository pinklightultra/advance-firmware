/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _ADV_USB_ROLE_MANAGER_H_
#define _ADV_USB_ROLE_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ADV_USB_CABLE_DETACHED = 0,
  ADV_USB_CABLE_DEVICE = 1,
  ADV_USB_CABLE_HOST = 2,
  ADV_USB_CABLE_UNRESOLVED = 3,
} AdvUsbCableState;

typedef enum {
  ADV_USB_TYPEC_ORIENT_NONE = 0,
  ADV_USB_TYPEC_ORIENT_CC1 = 1,
  ADV_USB_TYPEC_ORIENT_CC2 = 2,
  ADV_USB_TYPEC_ORIENT_FAULT = 3,
} AdvUsbTypeCOrientation;

typedef struct {
  uint8_t deviceId;
  uint8_t deviceType;
  uint8_t portRole;
  uint8_t control;
  uint8_t control1;
  uint8_t mask;
  uint8_t mask1;
  uint8_t status;
  uint8_t status1;
  uint8_t type;
  uint8_t interrupt;
  uint8_t interrupt1;
  bool attached;
  bool source;
  bool sink;
  bool vbusOk;
  bool fault;
  bool remedy;
  AdvUsbTypeCOrientation orient;
  AdvUsbCableState cableState;
  uint16_t interruptCause;
} AdvUsbTypeCStatus;

static inline bool
advUsbStatusRequestsHostPower(const AdvUsbTypeCStatus *status) {
  return (status != 0) && (status->cableState == ADV_USB_CABLE_HOST);
}

bool advUsbRoleManagerInit(void);
void advUsbRoleManagerService(void);
bool advUsbRoleManagerIsReady(void);
const AdvUsbTypeCStatus *advUsbRoleManagerGetStatus(void);

#ifdef __cplusplus
}
#endif

#endif
