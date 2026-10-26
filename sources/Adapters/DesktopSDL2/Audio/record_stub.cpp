/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc. & contributors
 *
 * record_stub.cpp - Desktop stubs for audio recording
 */

#include "Adapters/adv/audio/record.h"

uint16_t recordBuffer[RECORD_BUFFER_SIZE] = {0};

void Record(void *) {}

bool StartRecording(const char *filename, uint8_t threshold, uint32_t milliseconds) {
  return false;
}

void StopRecording() {}
void CancelRecording() {}
void RequestStopRecording() {}
bool WaitForRecordingStop(uint32_t timeoutMs) { return true; }
void FinishStopRecording() {}
void StartMonitoring() {}
void StopMonitoring() {}
void SetInputSource(RecordSource source) {}
void SetLineInGain(int32_t gainDb) {}
void SetMicGain(int32_t gainDb) {}
bool IsRecordingActive() { return false; }
bool IsSavingRecording() { return false; }
uint8_t GetSavingProgressPercent() { return 0; }
bool DidLastRecordingCaptureAudio() { return false; }
