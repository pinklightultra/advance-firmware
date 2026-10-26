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
#ifndef _RECORD_H_
#define _RECORD_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef _DESKTOP_
#ifndef _DESKTOP_
#ifndef _DESKTOP_
#include "FreeRTOS.h"
#include "semphr.h"
#endif
#endif
#endif

#define MAX_INT32 0xFFFFFFFF

#define RECORD_BUFFER_SIZE 35280 // Recording buffer is 200ms long
extern uint16_t recordBuffer[RECORD_BUFFER_SIZE];

#define LINEIN_GAIN_MINDB -6
#define LINEIN_GAIN_MAXDB 24
#define MIC_GAIN_MINDB 0  // can be negitive but mic level is already very low
#define MIC_GAIN_MAXDB 20 // per TLV320 datasheet max gain is 20dB

#ifndef _DESKTOP_
#ifndef _DESKTOP_
#ifndef _DESKTOP_
static StackType_t RecordStack[1024];
static StaticTask_t RecordTCB;
extern TaskHandle_t RecordHandle;
#endif
#endif
#endif

enum RecordSource { AllOff, LineIn, Mic, USBIn };

void Record(void *);
bool StartRecording(const char *filename, uint8_t threshold,

                    uint32_t milliseconds);
void StopRecording();
// Stop an in progress recording and throw it away instead of saving it to the
// sdcard, deleting the file if writing it out had already started. Used when
// there isnt time to save, eg. power off. Does not block.
void CancelRecording();
void RequestStopRecording();
bool WaitForRecordingStop(uint32_t timeoutMs);
void FinishStopRecording();
void StartMonitoring();
void StopMonitoring();
void SetInputSource(RecordSource source);
void SetLineInGain(int32_t gainDb);
void SetMicGain(int32_t gainDb);
bool IsRecordingActive();
bool IsSavingRecording();
uint8_t GetSavingProgressPercent();
// True only if the last recording captured samples and was persisted to disk.
bool DidLastRecordingCaptureAudio();

#endif
