/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _VOICE_ADSR_H_
#define _VOICE_ADSR_H_

class VoiceADSR {
public:
  VoiceADSR();

  void Reset();
  void SetParameters(int attack, int decay, int sustain, int release);

  void NoteOn();
  void NoteOff();
  void Kill();

  float Advance();
  struct LinearSegment {
    int samples;
    float firstValue;
    float step;
  };
  LinearSegment AdvanceBlock(int samples);
  void AdvanceSilent(int samples);

  float GetValue() const { return value_; }
  bool IsIdle() const { return state_ == State::Idle; }

private:
  enum class State { Idle, Attack, Decay, Sustain, Release };

  void updateStageSlope();
  static int tableIndex(int value);
  static float attackStep(int value);
  static float decayReleasePull(int value);
  void resetSegment();
  LinearSegment advanceSegmentBlock(int samples, float target, float pull);

  void enterDecay();
  void enterSustain();
  void enterRelease();

  int attack_;
  int decay_;
  int sustain_;
  int release_;

  State state_;
  float value_;
  float sustainLevel_;
  float attackStep_;
  float decayPull_;
  float releasePull_;
  float segmentStep_;
  int segmentSamplesRemaining_;
};

#endif
