/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _AUDIO_VOICE_H_
#define _AUDIO_VOICE_H_

#include "Application/Audio/AudioBufferLayout.h"
#include "Application/Audio/AudioSample.h"
#include "Application/Instruments/I_Instrument.h"
#include "BitcrushInsertEffect.h"
#include "ClipInsertEffect.h"
#include "DCOffsetInsertEffect.h"
#include "DetuneInsertEffect.h"
#include "DownsampleInsertEffect.h"
#include "Externals/etl/include/etl/variant_pool.h"
#include "Externals/etl/include/etl/vector.h"
#include "InsertEffectDefinitions.h"
#include "InsertEffectSlot.h"
#include "LFOProcessor.h"
#include "OverdriveInsertEffect.h"
#include "PassthroughInsertEffect.h"
#include "RectifyInsertEffect.h"
#include "SlewInsertEffect.h"
#include "VoiceADSR.h"
#include <cstdint>

struct DWTProfileStats;
class AccumulatingDWTProfile;

// Exposes the aggregate voice profile so the channel wet/dry pass can
// contribute its cost to the overall voice total.
DWTProfileStats *GetVoiceRenderProfileStats();

class AudioVoice {
public:
  explicit AudioVoice(int channel);

  bool StartInstrument(I_Instrument *instrument, unsigned char note,
                       bool trigger);
  bool ProcessCommand(TrackerCommand command, ushort value);
  void ReleaseInstrument();
  void StopInstrument();
  void Reset();

  I_Instrument *GetInstrument() const { return source_; }

  bool Render(AudioSample *buffer, int samplecount, bool updateTick);

private:
  static constexpr int InsertSlotCount = 2;
  using InsertEffectPool =
      etl::variant_pool<InsertSlotCount, PassthroughInsertEffect,
                        OverdriveInsertEffect, BitcrushInsertEffect,
                        DownsampleInsertEffect, SlewInsertEffect,
                        RectifyInsertEffect>;

  struct ParameterRefs {
    Variable *bandpassCutoff_ = nullptr;
    Variable *bandpassMix_ = nullptr;
    Variable *filterCutoff_ = nullptr;
    Variable *filterResonance_ = nullptr;
    Variable *filterType_ = nullptr;
    Variable *ampAttack_ = nullptr;
    Variable *ampDecay_ = nullptr;
    Variable *ampSustain_ = nullptr;
    Variable *ampRelease_ = nullptr;
    Variable *triggerLength_ = nullptr;
    Variable *filterAttack_ = nullptr;
    Variable *filterDecay_ = nullptr;
    Variable *filterSustain_ = nullptr;
    Variable *filterRelease_ = nullptr;
    Variable *filterEnvAmount_ = nullptr;
    Variable *amp_ = nullptr;
    Variable *pan_ = nullptr;
    Variable *insertType1_ = nullptr;
    Variable *insertAmount1_ = nullptr;
    Variable *insertPosition1_ = nullptr;
    Variable *insertType2_ = nullptr;
    Variable *insertAmount2_ = nullptr;
    Variable *insertPosition2_ = nullptr;
  };

  struct BaseWidthState {
    float highpassLow_ = 0.0f;
    float lowpassOut_ = 0.0f;
  };

  struct StandardFilterState {
    float ic1eq_ = 0.0f;
    float ic2eq_ = 0.0f;
  };

  struct FilterMorphState {
    int value_ = -1;
    float inputWeight_ = 0.0f;
    float bandpassBaseWeight_ = 0.0f;
    float highpassFeedbackWeight_ = 0.0f;
    float lowpassWeight_ = 1.0f;
  };

  struct StageLFOMasks {
    uint8_t preInsert_ = 0;
    uint8_t bandpass_ = 0;
    uint8_t standardFilter_ = 0;
    uint8_t amp_ = 0;
    uint8_t pan_ = 0;
    uint8_t postInsert_ = 0;
  };

  struct CommandValueState {
    float current_ = 0.0f;
    float target_ = 0.0f;
    float step_ = 0.0f;
    bool overrideActive_ = false;
    bool rampActive_ = false;
  };

  void refreshParameterRefs();
  void refreshStageLFOMasks();
  void resetTriggerRelease();
  void advanceTriggerRelease(bool updateTick);
  void resetCommandState();
  void resetCommandValue(CommandValueState &state);
  void setCommandImmediate(CommandValueState &state, float value);
  float getCommandStartValue(const CommandValueState &state, Variable *variable,
                             int defaultValue) const;
  void setCommandRamp(CommandValueState &state, float target, float speedParam,
                      Variable *variable, int defaultValue,
                      bool clampPanValue = false);
  void cancelCommandRamp(CommandValueState &state);
  void advanceCommandRamp(CommandValueState &state, int samplecount);
  void advanceCommandRamps(int samplecount);
  float getCommandRampStep(float target, float start, float speedParam) const;
  void resetStageState();
  void updateFilterEnvelopeParameters();
  void updateAmpEnvelopeParameters();
  void syncFilterType();
  void runStage(AccumulatingDWTProfile &chainProfile,
                AccumulatingDWTProfile &lfoProfile,
                AccumulatingDWTProfile &stageProfile, uint8_t lfoMask,
                AudioSample *inputBuffer, AudioBufferLayout inputLayout,
                AudioSample *outputBuffer, AudioBufferLayout outputLayout,
                int samplecount,
                void (AudioVoice::*processStage)(AudioSample *, AudioSample *,
                                                 int, AudioBufferLayout,
                                                 AudioBufferLayout),
                void (AudioVoice::*beforeStage)() = nullptr);
  int getVoiceValue(LFOTarget target, Variable *variable, int defaultValue,
                    int minValue, int maxValue) const;

  void processPreInsertStage(AudioSample *input, AudioSample *output,
                             int samplecount, AudioBufferLayout inputLayout,
                             AudioBufferLayout outputLayout);
  void processBandpass(AudioSample *input, AudioSample *output, int samplecount,
                       AudioBufferLayout inputLayout,
                       AudioBufferLayout outputLayout);
  void processStandardFilter(AudioSample *input, AudioSample *output,
                             int samplecount, AudioBufferLayout inputLayout,
                             AudioBufferLayout outputLayout);
  void processAmp(AudioSample *input, AudioSample *output, int samplecount,
                  AudioBufferLayout inputLayout,
                  AudioBufferLayout outputLayout);
  void processPan(AudioSample *input, AudioSample *output, int samplecount,
                  AudioBufferLayout inputLayout,
                  AudioBufferLayout outputLayout);
  void processPostInsertStage(AudioSample *input, AudioSample *output,
                              int samplecount, AudioBufferLayout inputLayout,
                              AudioBufferLayout outputLayout);
  void processInsertChain(etl::ivector<InsertEffectSlot *> &chain,
                          AudioSample *buffer, int samplecount,
                          AudioBufferLayout layout);
  InsertEffectProcessor *createInsertEffect(int insertType);
  void destroyInsertEffectSlot(int slot);
  void clearInsertEffects();
  void syncInsertEffects();
  bool syncInsertEffectSlot(int slot, Variable *type, Variable *position);
  void rebuildInsertChains();

  int channel_;
  I_Instrument *source_;
  ParameterRefs params_;
  InsertEffectPool insertEffectPool_;
  InsertEffectSlot insertEffectSlots_[InsertSlotCount];
  int insertTypes_[InsertSlotCount];
  int insertPositions_[InsertSlotCount];
  etl::vector<InsertEffectSlot *, InsertSlotCount> preInsertChain_;
  etl::vector<InsertEffectSlot *, InsertSlotCount> postInsertChain_;
  BaseWidthState bandpassState_[2];
  StandardFilterState standardFilterState_[2];
  FilterMorphState filterMorph_;
  StageLFOMasks stageLfoMasks_;
  CommandValueState ampCommand_;
  CommandValueState panCommand_;
  CommandValueState filterCutoffCommand_;
  CommandValueState filterResonanceCommand_;
  ChannelLFOBank lfoBank_;
  VoiceADSR ampEnvelope_;
  VoiceADSR filterEnvelope_;
  uint16_t triggerReleaseSlicesRemaining_;
  bool releasePending_;
};

#endif
