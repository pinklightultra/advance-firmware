#ifndef _DESKTOP_BRIDGE_H_
#define _DESKTOP_BRIDGE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

bool DesktopBridge_IsEnabled(void);
void DesktopBridge_Enable(void);
void DesktopBridge_Init(void);
void DesktopBridge_SendFrame(const uint16_t *fb, int width, int height);
void DesktopBridge_ProcessButtonMask(uint16_t newMask);
void DesktopBridge_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // _DESKTOP_BRIDGE_H_
