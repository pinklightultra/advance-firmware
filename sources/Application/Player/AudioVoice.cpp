/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "AudioVoice.h"
#include "Application/Player/SyncMaster.h"
#include "AudioVoiceFilterGLUT.h"
#if !defined(HOST_TEST)
#include "Application/Mixer/MixerService.h"
#endif
#include "Services/Audio/AudioDriver.h"
#include "System/Profiler/DWTProfiler.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {
constexpr int kEngineSampleRate = 44100;
constexpr int kLFOControlQuantumSamples = 24; // ~500us
constexpr float kMinFilterFrequencyHz = 20.0f;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kEnvelopeEpsilon = 1.0e-6f;
constexpr int kCommandKRateSampleCount = 100;
constexpr uint16_t kProfileOrderLegacySample = 10;
constexpr uint16_t kProfileOrderVoiceSource = 20;
constexpr uint16_t kProfileOrderVoicePreInsert = 29;
constexpr uint16_t kProfileOrderVoiceBandpass = 30;
constexpr uint16_t kProfileOrderVoiceFilter = 31;
constexpr uint16_t kProfileOrderVoiceAmp = 32;
constexpr uint16_t kProfileOrderVoicePostInsert = 34;
constexpr uint16_t kProfileOrderVoicePan = 35;
constexpr uint16_t kProfileOrderVoiceLFO = 36;
constexpr uint16_t kProfileOrderVoiceChain = 40;
constexpr uint16_t kProfileOrderVoiceTotal = 50;
constexpr int kMaxTriggerLengthSlices = 96;

DWTProfileStats gSourceRenderProfile = {" voice source"};
DWTProfileStats gLegacySampleRenderProfile = {"legacy sample"};
DWTProfileStats gPreInsertProfile = {"  insert pre"};
DWTProfileStats gBandpassProfile = {"  voice bw"};
DWTProfileStats gStandardFilterProfile = {"  voice filter"};
DWTProfileStats gAmpProfile = {"  voice amp"};
DWTProfileStats gPostInsertProfile = {"  insert post"};
DWTProfileStats gPanProfile = {"  voice pan"};
DWTProfileStats gVoiceChainProfile = {" voice chain"};
DWTProfileStats gVoiceLFOProfile = {"  chan lfo"};
DWTProfileStats gVoiceRenderProfile = {"voice total"};

inline int getProfileVoiceCount() {
#if defined(HOST_TEST)
  return SONG_CHANNEL_COUNT_STEREO;
#else
  MixerService *mixerService = MixerService::GetInstance();
  Project *project = mixerService ? mixerService->GetProject() : nullptr;
  if (project == nullptr) {
    return SONG_CHANNEL_COUNT_STEREO;
  }

  return project->song_.GetChannelCount();
#endif
}

inline float lookupFilterG(float normalized) {
  const float position =
      std::clamp(normalized, 0.0f, 1.0f) *
      static_cast<float>(AudioVoiceFilterGLUT::kFilterGLUTSize);
  const size_t index = std::min(static_cast<size_t>(position),
                                AudioVoiceFilterGLUT::kFilterGLUTSize - 1);
  const float fraction = position - static_cast<float>(index);
  return AudioVoiceFilterGLUT::kFilterGValues[index] +
         (AudioVoiceFilterGLUT::kFilterGValues[index + 1] -
          AudioVoiceFilterGLUT::kFilterGValues[index]) *
             fraction;
}

void registerVoiceProfiles() {
  static bool registered = false;
  if (registered) {
    return;
  }

  gLegacySampleRenderProfile.displayOrder_ = kProfileOrderLegacySample;
  gSourceRenderProfile.displayOrder_ = kProfileOrderVoiceSource;
  gPreInsertProfile.displayOrder_ = kProfileOrderVoicePreInsert;
  gBandpassProfile.displayOrder_ = kProfileOrderVoiceBandpass;
  gStandardFilterProfile.displayOrder_ = kProfileOrderVoiceFilter;
  gAmpProfile.displayOrder_ = kProfileOrderVoiceAmp;
  gPanProfile.displayOrder_ = kProfileOrderVoicePan;
  gPostInsertProfile.displayOrder_ = kProfileOrderVoicePostInsert;
  gVoiceLFOProfile.displayOrder_ = kProfileOrderVoiceLFO;
  gVoiceChainProfile.displayOrder_ = kProfileOrderVoiceChain;
  gVoiceRenderProfile.displayOrder_ = kProfileOrderVoiceTotal;

  RegisterDWTProfileStats(&gLegacySampleRenderProfile);
  RegisterDWTProfileStats(&gSourceRenderProfile);
  RegisterDWTProfileStats(&gPreInsertProfile);
  RegisterDWTProfileStats(&gBandpassProfile);
  RegisterDWTProfileStats(&gStandardFilterProfile);
  RegisterDWTProfileStats(&gAmpProfile);
  RegisterDWTProfileStats(&gPostInsertProfile);
  RegisterDWTProfileStats(&gPanProfile);
  RegisterDWTProfileStats(&gVoiceChainProfile);
  RegisterDWTProfileStats(&gVoiceLFOProfile);
  RegisterDWTProfileStats(&gVoiceRenderProfile);
  registered = true;
}

inline int clampPan(int value) { return std::clamp(value, 0, 0xFE); }

inline float byteToUnit(int value) {
  return std::clamp(value, 0, 0xFF) / 255.0f;
}

inline float bipolarDepth(int value) {
  return std::clamp(value, -64, 64) / 64.0f;
}

inline float mapCutoffHz(int value) {
  return kMinFilterFrequencyHz *
         std::pow(std::min(kEngineSampleRate * 0.45f, 18000.0f) /
                      kMinFilterFrequencyHz,
                  byteToUnit(value));
}

inline float maxCutoffHz() {
  return std::min(kEngineSampleRate * 0.45f, 18000.0f);
}

inline float onePoleAlpha(float cutoffHz) {
  if (cutoffHz <= 0.0f) {
    return 0.0f;
  }

  const float normalized =
      std::exp((-2.0f * kPi * cutoffHz) / kEngineSampleRate);
  return std::clamp(1.0f - normalized, 0.0f, 1.0f);
}

inline float mapResonanceQ(int value) {
  const float norm = byteToUnit(value);
  return 0.707f + (norm * norm) * 15.0f;
}

inline AudioSample panGainLeft(int pan) {
  const float norm = std::clamp(pan / 254.0f, 0.0f, 1.0f);
  const float gain = (norm <= 0.5f) ? 1.0f : (2.0f * (1.0f - norm));
  return std::clamp(gain, 0.0f, 1.0f);
}

inline AudioSample panGainRight(int pan) {
  const float norm = std::clamp(pan / 254.0f, 0.0f, 1.0f);
  const float gain = (norm >= 0.5f) ? 1.0f : (2.0f * norm);
  return std::clamp(gain, 0.0f, 1.0f);
}

} // namespace

DWTProfileStats *GetVoiceRenderProfileStats() { return &gVoiceRenderProfile; }

AudioVoice::AudioVoice(int channel)
    : channel_(channel), source_(nullptr), lfoBank_(channel),
      triggerReleaseSlicesRemaining_(0), releasePending_(false) {
  registerVoiceProfiles();
  clearInsertEffects();
  resetCommandState();
  resetStageState();
}

bool AudioVoice::StartInstrument(I_Instrument *instrument, unsigned char note,
                                 bool trigger) {
  const bool continueVoice = (source_ == instrument) && !trigger;

  if (source_ && !continueVoice) {
    StopInstrument();
  }

  if (!instrument) {
    return false;
  }

  if (!instrument->Start(channel_, note, trigger)) {
    if (continueVoice) {
      StopInstrument();
    } else {
      source_ = nullptr;
    }
    return false;
  }

  source_ = instrument;
  lfoBank_.SetInstrument(source_);
  lfoBank_.OnNoteStart(trigger);
  refreshParameterRefs();
  updateFilterEnvelopeParameters();
  updateAmpEnvelopeParameters();

  if (!continueVoice) {
    resetCommandState();
    resetTriggerRelease();
    releasePending_ = false;
    ampEnvelope_.NoteOn();
    filterEnvelope_.NoteOn();
  }

  if (trigger) {
    resetStageState();
  }
  return true;
}

bool AudioVoice::ProcessCommand(TrackerCommand command, ushort value) {
  if (!source_ || source_->GetType() == IT_SAMPLE) {
    return false;
  }

  switch (command) {
  case TrackerCommand::InstrumentCommandVolume:
    setCommandRamp(ampCommand_, static_cast<float>(value & 0xFF),
                   static_cast<float>(value >> 8), params_.amp_, 0xFF);
    return true;

  case TrackerCommand::InstrumentCommandPan:
    setCommandRamp(panCommand_, static_cast<float>(value & 0xFF),
                   static_cast<float>(value >> 8), params_.pan_, 0x7F, true);
    return true;

  case TrackerCommand::InstrumentCommandFilterCut:
    setCommandRamp(filterCutoffCommand_, static_cast<float>(value & 0xFF),
                   static_cast<float>(value >> 8), params_.filterCutoff_, 0xFF);
    return true;

  case TrackerCommand::InstrumentCommandFilterResonance:
    setCommandRamp(filterResonanceCommand_, static_cast<float>(value & 0xFF),
                   static_cast<float>(value >> 8), params_.filterResonance_, 0);
    return true;

  case TrackerCommand::InstrumentCommandLowPassFilter:
    setCommandImmediate(filterCutoffCommand_, static_cast<float>(value >> 8));
    setCommandImmediate(filterResonanceCommand_,
                        static_cast<float>(value & 0xFF));
    cancelCommandRamp(filterCutoffCommand_);
    cancelCommandRamp(filterResonanceCommand_);
    return true;

  case TrackerCommand::InstrumentCommandGateLength:
    triggerReleaseSlicesRemaining_ = static_cast<uint16_t>((value & 0xFF) + 1);
    return true;

  default:
    return false;
  }
}

void AudioVoice::ReleaseInstrument() {
  if (!source_) {
    return;
  }
  triggerReleaseSlicesRemaining_ = 0;

  // Legacy sample instruments are a deliberate exception: they still flow
  // through AudioVoice ownership on the channel, but they do not use the voice
  // envelope/FX pipeline and should stop immediately on release.
  if (source_->GetType() == IT_SAMPLE) {
    StopInstrument();
    return;
  }

  if (releasePending_) {
    return;
  }

  source_->Release(channel_);
  releasePending_ = true;
  ampEnvelope_.NoteOff();
  filterEnvelope_.NoteOff();
}

void AudioVoice::StopInstrument() {
  if (source_) {
    source_->Stop(channel_);
    source_ = nullptr;
  }
  lfoBank_.ClearInstrument();
  params_ = {};
  stageLfoMasks_ = {};
  clearInsertEffects();
  releasePending_ = false;
  triggerReleaseSlicesRemaining_ = 0;
  resetCommandState();
  ampEnvelope_.Kill();
  filterEnvelope_.Kill();
  resetStageState();
}

void AudioVoice::Reset() {
  source_ = nullptr;
  lfoBank_.Reset();
  lfoBank_.ClearInstrument();
  params_ = {};
  stageLfoMasks_ = {};
  clearInsertEffects();
  releasePending_ = false;
  triggerReleaseSlicesRemaining_ = 0;
  resetCommandState();
  ampEnvelope_.Reset();
  filterEnvelope_.Reset();
  resetStageState();
}

bool AudioVoice::Render(AudioSample *buffer, int samplecount, bool updateTick) {
  if (!source_) {
    return false;
  }

  constexpr int sampleRate = kEngineSampleRate;
  const int profileVoiceCount = getProfileVoiceCount();

  // Legacy sample instruments are a deliberate exception: render only the
  // instrument source here and skip the AudioVoice profiler/processing chain.
  if (source_->GetType() == IT_SAMPLE) {
    ScopedDWTProfile legacySampleProfile(&gLegacySampleRenderProfile,
                                         samplecount, sampleRate,
                                         profileVoiceCount);
    return source_->Render(channel_, buffer, samplecount, updateTick);
  }

  const AudioBufferLayout sourceLayout =
      source_->GetAudioBufferLayout(channel_);
  // Reuse the upper half of the stereo render buffer as packed mono scratch.
  // Pan widens forward from that region back into the front half, so each mono
  // source sample is consumed before its destination writes can overlap it.
  AudioSample *sourceBuffer = sourceLayout == AudioBufferLayout::MonoPacked
                                  ? buffer + samplecount
                                  : buffer;

  ScopedDWTProfile voiceProfile(&gVoiceRenderProfile, samplecount, sampleRate,
                                profileVoiceCount);
  {
    ScopedDWTProfile sourceProfile(&gSourceRenderProfile, samplecount,
                                   sampleRate, profileVoiceCount);
    if (!source_->Render(channel_, sourceBuffer, samplecount, updateTick)) {
      // Today, "source finished" is treated as "voice finished", so the whole
      // voice is torn down immediately. If we later want tail-capable insert
      // effects (delay/reverb/etc.) to ring out after the source stops, this
      // needs to change to keep the voice chain alive until those stages go
      // idle too.
      source_ = nullptr;
      params_ = {};
      stageLfoMasks_ = {};
      clearInsertEffects();
      releasePending_ = false;
      triggerReleaseSlicesRemaining_ = 0;
      ampEnvelope_.Kill();
      filterEnvelope_.Kill();
      lfoBank_.ClearInstrument();
      resetStageState();
      return false;
    }
  }

  {
    AudioSample *prePanBuffer = sourceBuffer;
    const AudioBufferLayout prePanLayout = sourceLayout;
    AccumulatingDWTProfile chainProfile(&gVoiceChainProfile, samplecount,
                                        sampleRate, profileVoiceCount);
    AccumulatingDWTProfile preInsertProfile(&gPreInsertProfile, samplecount,
                                            sampleRate, profileVoiceCount);
    AccumulatingDWTProfile bandpassProfile(&gBandpassProfile, samplecount,
                                           sampleRate, profileVoiceCount);
    AccumulatingDWTProfile standardFilterProfile(
        &gStandardFilterProfile, samplecount, sampleRate, profileVoiceCount);
    AccumulatingDWTProfile ampProfile(&gAmpProfile, samplecount, sampleRate,
                                      profileVoiceCount);
    AccumulatingDWTProfile postInsertProfile(&gPostInsertProfile, samplecount,
                                             sampleRate, profileVoiceCount);
    AccumulatingDWTProfile panProfile(&gPanProfile, samplecount, sampleRate,
                                      profileVoiceCount);
    AccumulatingDWTProfile lfoProfile(&gVoiceLFOProfile, samplecount,
                                      sampleRate, profileVoiceCount);

    syncInsertEffects();
    syncFilterType();

    runStage(chainProfile, lfoProfile, preInsertProfile,
             stageLfoMasks_.preInsert_, prePanBuffer, prePanLayout,
             prePanBuffer, prePanLayout, samplecount,
             &AudioVoice::processPreInsertStage);
    runStage(chainProfile, lfoProfile, bandpassProfile,
             stageLfoMasks_.bandpass_, prePanBuffer, prePanLayout, prePanBuffer,
             prePanLayout, samplecount, &AudioVoice::processBandpass);
    runStage(chainProfile, lfoProfile, standardFilterProfile,
             stageLfoMasks_.standardFilter_, prePanBuffer, prePanLayout,
             prePanBuffer, prePanLayout, samplecount,
             &AudioVoice::processStandardFilter,
             &AudioVoice::updateFilterEnvelopeParameters);
    runStage(chainProfile, lfoProfile, ampProfile, stageLfoMasks_.amp_,
             prePanBuffer, prePanLayout, prePanBuffer, prePanLayout,
             samplecount, &AudioVoice::processAmp,
             &AudioVoice::updateAmpEnvelopeParameters);
    runStage(chainProfile, lfoProfile, postInsertProfile,
             stageLfoMasks_.postInsert_, prePanBuffer, prePanLayout,
             prePanBuffer, prePanLayout, samplecount,
             &AudioVoice::processPostInsertStage);
    runStage(chainProfile, lfoProfile, panProfile, stageLfoMasks_.pan_,
             prePanBuffer, prePanLayout, buffer,
             AudioBufferLayout::StereoInterleaved, samplecount,
             &AudioVoice::processPan);

    // Voice command ramps now advance once per render slice so only LFO
    // targets can force stage chunking.
    advanceCommandRamps(samplecount);
  }

  advanceTriggerRelease(updateTick);

  if (releasePending_ && ampEnvelope_.IsIdle()) {
    if (source_) {
      source_->Stop(channel_);
      source_ = nullptr;
    }
    releasePending_ = false;
    triggerReleaseSlicesRemaining_ = 0;
    resetStageState();
  }
  return true;
}

void AudioVoice::refreshParameterRefs() {
  params_ = {};
  if (!source_) {
    return;
  }

  params_.bandpassCutoff_ = source_->FindVariable(FourCC::VoiceBandpassCutoff);
  params_.bandpassMix_ = source_->FindVariable(FourCC::VoiceBandpassMix);
  params_.filterCutoff_ = source_->FindVariable(FourCC::VoiceFilterCutoff);
  params_.filterResonance_ =
      source_->FindVariable(FourCC::VoiceFilterResonance);
  params_.filterType_ = source_->FindVariable(FourCC::VoiceFilterType);
  params_.ampAttack_ = source_->FindVariable(FourCC::VoiceAmpAttack);
  params_.ampDecay_ = source_->FindVariable(FourCC::VoiceAmpDecay);
  params_.ampSustain_ = source_->FindVariable(FourCC::VoiceAmpSustain);
  params_.ampRelease_ = source_->FindVariable(FourCC::VoiceAmpRelease);
  params_.triggerLength_ = source_->FindVariable(FourCC::VoiceTriggerLength);
  params_.filterAttack_ = source_->FindVariable(FourCC::VoiceFilterAttack);
  params_.filterDecay_ = source_->FindVariable(FourCC::VoiceFilterDecay);
  params_.filterSustain_ = source_->FindVariable(FourCC::VoiceFilterSustain);
  params_.filterRelease_ = source_->FindVariable(FourCC::VoiceFilterRelease);
  params_.filterEnvAmount_ =
      source_->FindVariable(FourCC::VoiceFilterEnvAmount);
  params_.amp_ = source_->FindVariable(FourCC::VoiceAmp);
  params_.pan_ = source_->FindVariable(FourCC::VoicePan);
  params_.insertType1_ = source_->FindVariable(GetVoiceInsertTypeFourCC(0));
  params_.insertAmount1_ = source_->FindVariable(GetVoiceInsertAmountFourCC(0));
  params_.insertPosition1_ =
      source_->FindVariable(GetVoiceInsertPositionFourCC(0));
  params_.insertType2_ = source_->FindVariable(GetVoiceInsertTypeFourCC(1));
  params_.insertAmount2_ = source_->FindVariable(GetVoiceInsertAmountFourCC(1));
  params_.insertPosition2_ =
      source_->FindVariable(GetVoiceInsertPositionFourCC(1));
  syncInsertEffects();
  syncFilterType();
  refreshStageLFOMasks();
}

void AudioVoice::resetTriggerRelease() {
  if (params_.triggerLength_) {
    triggerReleaseSlicesRemaining_ = static_cast<uint16_t>(std::clamp(
        params_.triggerLength_->GetInt(), 0, kMaxTriggerLengthSlices));
    if (triggerReleaseSlicesRemaining_ > 0) {
      triggerReleaseSlicesRemaining_++;
    }
  } else {
    triggerReleaseSlicesRemaining_ = 0;
  }
}

void AudioVoice::advanceTriggerRelease(bool updateTick) {
  if (!updateTick || releasePending_ || triggerReleaseSlicesRemaining_ == 0) {
    return;
  }

  triggerReleaseSlicesRemaining_--;
  if (triggerReleaseSlicesRemaining_ == 0) {
    ReleaseInstrument();
  }
}

void AudioVoice::resetCommandState() {
  resetCommandValue(ampCommand_);
  resetCommandValue(panCommand_);
  resetCommandValue(filterCutoffCommand_);
  resetCommandValue(filterResonanceCommand_);
}

void AudioVoice::resetCommandValue(CommandValueState &state) { state = {}; }

void AudioVoice::setCommandImmediate(CommandValueState &state, float value) {
  state.current_ = value;
  state.target_ = value;
  state.step_ = 0.0f;
  state.overrideActive_ = true;
  state.rampActive_ = false;
}

float AudioVoice::getCommandStartValue(const CommandValueState &state,
                                       Variable *variable,
                                       int defaultValue) const {
  if (state.overrideActive_) {
    return state.current_;
  }
  return static_cast<float>(variable ? variable->GetInt() : defaultValue);
}

void AudioVoice::setCommandRamp(CommandValueState &state, float target,
                                float speedParam, Variable *variable,
                                int defaultValue, bool clampPanValue) {
  if (clampPanValue && target >= 0xFF) {
    target = 0xFE;
  }

  const float start = getCommandStartValue(state, variable, defaultValue);
  const float step = getCommandRampStep(target, start, speedParam);

  state.current_ = start;
  state.target_ = target;
  state.step_ = step;
  state.overrideActive_ = true;
  state.rampActive_ = speedParam != 0.0f && step > 0.0f;

  if (!state.rampActive_) {
    state.current_ = target;
  }
}

void AudioVoice::cancelCommandRamp(CommandValueState &state) {
  state.rampActive_ = false;
  state.step_ = 0.0f;
}

void AudioVoice::advanceCommandRamp(CommandValueState &state, int samplecount) {
  if (!state.overrideActive_ || !state.rampActive_) {
    return;
  }

  if (state.step_ <= 0.0f) {
    state.current_ = state.target_;
    state.rampActive_ = false;
    return;
  }

  const float scaledStep =
      state.step_ * samplecount / static_cast<float>(kCommandKRateSampleCount);
  if (state.current_ < state.target_) {
    state.current_ = std::min(state.current_ + scaledStep, state.target_);
  } else {
    state.current_ = std::max(state.current_ - scaledStep, state.target_);
  }

  if (std::abs(state.current_ - state.target_) <= 1.0e-6f) {
    state.current_ = state.target_;
    state.rampActive_ = false;
  }
}

void AudioVoice::advanceCommandRamps(int samplecount) {
  advanceCommandRamp(filterCutoffCommand_, samplecount);
  advanceCommandRamp(filterResonanceCommand_, samplecount);
  advanceCommandRamp(ampCommand_, samplecount);
  advanceCommandRamp(panCommand_, samplecount);
}

float AudioVoice::getCommandRampStep(float target, float start,
                                     float speedParam) const {
  if (speedParam == 0.0f) {
    return 0.0f;
  }

  SyncMaster *syncMaster = SyncMaster::GetInstance();
  const float tickSampleCount =
      syncMaster ? syncMaster->GetTickSampleCount() : 0.0f;
  const float sampleCount = 4.0f * tickSampleCount;
  if (sampleCount <= 0.0f) {
    return 0.0f;
  }

  return std::abs(target - start) * kCommandKRateSampleCount / speedParam /
         sampleCount;
}

void AudioVoice::refreshStageLFOMasks() {
  stageLfoMasks_ = {};
  stageLfoMasks_.bandpass_ =
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceBandpassCutoff) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceBandpassMix);
  stageLfoMasks_.standardFilter_ =
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterCutoff) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterResonance) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterEnvAmount) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterAttack) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterDecay) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterSustain) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceFilterRelease);
  stageLfoMasks_.amp_ =
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceAmp) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceAmpAttack) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceAmpDecay) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceAmpSustain) |
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceAmpRelease);
  stageLfoMasks_.pan_ = lfoBank_.GetSlotMaskForTarget(LFOTarget::VoicePan);

  const uint8_t insert1Mask =
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceInsertAmount1);
  const uint8_t insert2Mask =
      lfoBank_.GetSlotMaskForTarget(LFOTarget::VoiceInsertAmount2);
  if (insertPositions_[0] == kVoiceInsertPositionPre) {
    stageLfoMasks_.preInsert_ |= insert1Mask;
  } else {
    stageLfoMasks_.postInsert_ |= insert1Mask;
  }
  if (insertPositions_[1] == kVoiceInsertPositionPre) {
    stageLfoMasks_.preInsert_ |= insert2Mask;
  } else {
    stageLfoMasks_.postInsert_ |= insert2Mask;
  }
}

void AudioVoice::resetStageState() {
  for (auto &state : bandpassState_) {
    state.highpassLow_ = 0.0f;
    state.lowpassOut_ = 0.0f;
  }
  for (auto &state : standardFilterState_) {
    state.ic1eq_ = 0.0f;
    state.ic2eq_ = 0.0f;
  }
  for (int slot = 0; slot < InsertSlotCount; ++slot) {
    insertEffectSlots_[slot].ResetState();
  }
}

void AudioVoice::updateFilterEnvelopeParameters() {
  filterEnvelope_.SetParameters(
      getVoiceValue(LFOTarget::VoiceFilterAttack, params_.filterAttack_, 0, 0,
                    0xFF),
      getVoiceValue(LFOTarget::VoiceFilterDecay, params_.filterDecay_, 0, 0,
                    0xFF),
      getVoiceValue(LFOTarget::VoiceFilterSustain, params_.filterSustain_, 0, 0,
                    0xFF),
      getVoiceValue(LFOTarget::VoiceFilterRelease, params_.filterRelease_, 0, 0,
                    0xFF));
}

void AudioVoice::updateAmpEnvelopeParameters() {
  ampEnvelope_.SetParameters(
      getVoiceValue(LFOTarget::VoiceAmpAttack, params_.ampAttack_, 0, 0, 0xFF),
      getVoiceValue(LFOTarget::VoiceAmpDecay, params_.ampDecay_, 0, 0, 0xFF),
      getVoiceValue(LFOTarget::VoiceAmpSustain, params_.ampSustain_, 0xFF, 0,
                    0xFF),
      getVoiceValue(LFOTarget::VoiceAmpRelease, params_.ampRelease_, 0, 0,
                    0xFF));
}

void AudioVoice::syncFilterType() {
  const int filterTypeValue = std::clamp(
      params_.filterType_ ? params_.filterType_->GetInt() : 0, 0, 0xFF);
  if (filterMorph_.value_ == filterTypeValue) {
    return;
  }

  const float filterType = byteToUnit(filterTypeValue);
  const float lowToBand = std::min(filterType * 2.0f, 1.0f);
  const float bandToHigh = std::max(filterType * 2.0f - 1.0f, 0.0f);
  const float lowBandAmount = 1.0f - bandToHigh;

  filterMorph_.value_ = filterTypeValue;
  filterMorph_.inputWeight_ = bandToHigh;
  filterMorph_.bandpassBaseWeight_ = lowBandAmount * lowToBand;
  filterMorph_.highpassFeedbackWeight_ = bandToHigh;
  filterMorph_.lowpassWeight_ = lowBandAmount * (1.0f - lowToBand) - bandToHigh;
}

void AudioVoice::runStage(
    AccumulatingDWTProfile &chainProfile, AccumulatingDWTProfile &lfoProfile,
    AccumulatingDWTProfile &stageProfile, uint8_t lfoMask,
    AudioSample *inputBuffer, AudioBufferLayout inputLayout,
    AudioSample *outputBuffer, AudioBufferLayout outputLayout, int samplecount,
    void (AudioVoice::*processStage)(AudioSample *, AudioSample *, int,
                                     AudioBufferLayout, AudioBufferLayout),
    void (AudioVoice::*beforeStage)()) {
  if (lfoMask == 0) {
    if (beforeStage) {
      (this->*beforeStage)();
    }
    chainProfile.Begin();
    stageProfile.Begin();
    (this->*processStage)(inputBuffer, outputBuffer, samplecount, inputLayout,
                          outputLayout);
    stageProfile.End();
    chainProfile.End();
    return;
  }

  int processedSamples = 0;
  const int inputStride = GetAudioBufferLayoutChannelCount(inputLayout);
  const int outputStride = GetAudioBufferLayoutChannelCount(outputLayout);
  while (processedSamples < samplecount) {
    const int chunkSize =
        std::min(kLFOControlQuantumSamples, samplecount - processedSamples);
    AudioSample *chunkInput = inputBuffer + processedSamples * inputStride;
    AudioSample *chunkOutput = outputBuffer + processedSamples * outputStride;
    if (beforeStage) {
      (this->*beforeStage)();
    }
    chainProfile.Begin();
    stageProfile.Begin();
    (this->*processStage)(chunkInput, chunkOutput, chunkSize, inputLayout,
                          outputLayout);
    stageProfile.End();

    if (lfoMask != 0) {
      lfoProfile.Begin();
      lfoBank_.AdvanceSlots(lfoMask, chunkSize);
      lfoProfile.End();
    }
    chainProfile.End();

    processedSamples += chunkSize;
  }
}

int AudioVoice::getVoiceValue(LFOTarget target, Variable *variable,
                              int defaultValue, int minValue,
                              int maxValue) const {
  const CommandValueState *commandState = nullptr;
  switch (target) {
  case LFOTarget::VoiceFilterCutoff:
    commandState = &filterCutoffCommand_;
    break;
  case LFOTarget::VoiceFilterResonance:
    commandState = &filterResonanceCommand_;
    break;
  case LFOTarget::VoiceAmp:
    commandState = &ampCommand_;
    break;
  case LFOTarget::VoicePan:
    commandState = &panCommand_;
    break;
  default:
    break;
  }

  int baseValue = variable ? variable->GetInt() : defaultValue;
  if (commandState && commandState->overrideActive_) {
    baseValue = static_cast<int>(std::lround(commandState->current_));
  }
  return lfoBank_.GetLiveModulatedValue(target, baseValue, minValue, maxValue);
}

void AudioVoice::processPreInsertStage(AudioSample *input, AudioSample *output,
                                       int samplecount,
                                       AudioBufferLayout inputLayout,
                                       AudioBufferLayout outputLayout) {
  (void)input;
  (void)outputLayout;
  processInsertChain(preInsertChain_, output, samplecount, inputLayout);
}

void AudioVoice::processBandpass(AudioSample *input, AudioSample *output,
                                 int samplecount, AudioBufferLayout inputLayout,
                                 AudioBufferLayout outputLayout) {
  (void)outputLayout;
  const int base = getVoiceValue(LFOTarget::VoiceBandpassCutoff,
                                 params_.bandpassCutoff_, 0, 0, 0xFF);
  const int width = getVoiceValue(LFOTarget::VoiceBandpassMix,
                                  params_.bandpassMix_, 0xFF, 0, 0xFF);
  const bool highpassActive = base > 0;
  const bool lowpassActive = width < 0xFF;
  const float maxHz = maxCutoffHz();
  const float baseHz = highpassActive ? mapCutoffHz(base) : 0.0f;
  float lowpassHz = maxHz;

  if (lowpassActive) {
    if (!highpassActive) {
      lowpassHz = mapCutoffHz(width);
    } else {
      lowpassHz = baseHz + byteToUnit(width) * (maxHz - baseHz);
    }
  }

  const float highpassAlpha = highpassActive ? onePoleAlpha(baseHz) : 0.0f;
  const float lowpassAlpha = lowpassActive ? onePoleAlpha(lowpassHz) : 0.0f;

  if (inputLayout == AudioBufferLayout::MonoPacked) {
    BaseWidthState &state = bandpassState_[0];
    for (int i = 0; i < samplecount; ++i) {
      const float inputSample = input[i];
      float stageOutput = inputSample;

      if (highpassActive) {
        state.highpassLow_ +=
            highpassAlpha * (inputSample - state.highpassLow_);
        stageOutput = inputSample - state.highpassLow_;
      } else {
        state.highpassLow_ = inputSample;
      }

      if (lowpassActive) {
        state.lowpassOut_ += lowpassAlpha * (stageOutput - state.lowpassOut_);
        stageOutput = state.lowpassOut_;
      } else {
        state.lowpassOut_ = stageOutput;
      }

      output[i] = stageOutput;
    }
    return;
  }

  for (int i = 0; i < samplecount; ++i) {
    for (int ch = 0; ch < 2; ++ch) {
      const int index = i * 2 + ch;
      BaseWidthState &state = bandpassState_[ch];
      const float inputSample = input[index];
      float stageOutput = inputSample;

      if (highpassActive) {
        state.highpassLow_ +=
            highpassAlpha * (inputSample - state.highpassLow_);
        stageOutput = inputSample - state.highpassLow_;
      } else {
        state.highpassLow_ = inputSample;
      }

      if (lowpassActive) {
        state.lowpassOut_ += lowpassAlpha * (stageOutput - state.lowpassOut_);
        stageOutput = state.lowpassOut_;
      } else {
        state.lowpassOut_ = stageOutput;
      }

      output[index] = stageOutput;
    }
  }
}

void AudioVoice::processStandardFilter(AudioSample *input, AudioSample *output,
                                       int samplecount,
                                       AudioBufferLayout inputLayout,
                                       AudioBufferLayout outputLayout) {
  (void)outputLayout;
  // Note: Filter defaults are musically "open", but not technically a fully
  // transparent bypass because the lowpass filter remains in the signal path.
  const float q = mapResonanceQ(getVoiceValue(
      LFOTarget::VoiceFilterResonance, params_.filterResonance_, 0, 0, 0xFF));
  const float k = 1.0f / q;
  const float baseCutoff = byteToUnit(getVoiceValue(
      LFOTarget::VoiceFilterCutoff, params_.filterCutoff_, 0xFF, 0, 0xFF));
  const float inputWeight = filterMorph_.inputWeight_;
  const float bandpassWeight = filterMorph_.bandpassBaseWeight_ -
                               filterMorph_.highpassFeedbackWeight_ * k;
  const float lowpassWeight = filterMorph_.lowpassWeight_;
  const float envAmount = bipolarDepth(getVoiceValue(
      LFOTarget::VoiceFilterEnvAmount, params_.filterEnvAmount_, 0, -64, 64));

  if (inputLayout == AudioBufferLayout::MonoPacked) {
    StandardFilterState &state = standardFilterState_[0];
    int processedSamples = 0;
    while (processedSamples < samplecount) {
      const VoiceADSR::LinearSegment envelopeSegment =
          filterEnvelope_.AdvanceBlock(samplecount - processedSamples);
      if (envelopeSegment.samples <= 0) {
        break;
      }

      float envValue = envelopeSegment.firstValue;
      const int segmentEnd = processedSamples + envelopeSegment.samples;
      for (; processedSamples < segmentEnd; ++processedSamples) {
        const float cutoff =
            std::clamp(baseCutoff + envAmount * envValue, 0.0f, 1.0f);
        const float g = lookupFilterG(cutoff);
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float inputSample = input[processedSamples];

        const float v3 = inputSample - state.ic2eq_;
        const float v1 = a1 * state.ic1eq_ + a2 * v3;
        const float v2 = state.ic2eq_ + a2 * state.ic1eq_ + a3 * v3;
        state.ic1eq_ = 2.0f * v1 - state.ic1eq_;
        state.ic2eq_ = 2.0f * v2 - state.ic2eq_;

        output[processedSamples] = inputWeight * inputSample +
                                   bandpassWeight * v1 + lowpassWeight * v2;
        envValue += envelopeSegment.step;
      }
    }
    return;
  }

  int processedSamples = 0;
  while (processedSamples < samplecount) {
    const VoiceADSR::LinearSegment envelopeSegment =
        filterEnvelope_.AdvanceBlock(samplecount - processedSamples);
    if (envelopeSegment.samples <= 0) {
      break;
    }

    float envValue = envelopeSegment.firstValue;
    const int segmentEnd = processedSamples + envelopeSegment.samples;
    for (; processedSamples < segmentEnd; ++processedSamples) {
      const float cutoff =
          std::clamp(baseCutoff + envAmount * envValue, 0.0f, 1.0f);
      // The constant table maps normalized cutoff through the exact exponential
      // frequency curve and tan(pi * f / fs) TPT coefficient transform.
      const float g = lookupFilterG(cutoff);
      const float a1 = 1.0f / (1.0f + g * (g + k));
      const float a2 = g * a1;
      const float a3 = g * a2;

      for (int ch = 0; ch < 2; ++ch) {
        const int index = processedSamples * 2 + ch;
        StandardFilterState &state = standardFilterState_[ch];
        const float inputSample = input[index];

        const float v3 = inputSample - state.ic2eq_;
        const float v1 = a1 * state.ic1eq_ + a2 * v3;
        const float v2 = state.ic2eq_ + a2 * state.ic1eq_ + a3 * v3;
        state.ic1eq_ = 2.0f * v1 - state.ic1eq_;
        state.ic2eq_ = 2.0f * v2 - state.ic2eq_;

        output[index] = inputWeight * inputSample + bandpassWeight * v1 +
                        lowpassWeight * v2;
      }
      envValue += envelopeSegment.step;
    }
  }
}

void AudioVoice::processAmp(AudioSample *input, AudioSample *output,
                            int samplecount, AudioBufferLayout inputLayout,
                            AudioBufferLayout outputLayout) {
  (void)outputLayout;
  const int amp =
      getVoiceValue(LFOTarget::VoiceAmp, params_.amp_, 0xFF, 0, 0xFF);
  const float baseGain = byteToUnit(amp);
  const int channelCount = GetAudioBufferLayoutChannelCount(inputLayout);

  int processedSamples = 0;
  while (processedSamples < samplecount) {
    const VoiceADSR::LinearSegment envelopeSegment =
        ampEnvelope_.AdvanceBlock(samplecount - processedSamples);
    if (envelopeSegment.samples <= 0) {
      break;
    }

    if (releasePending_ && ampEnvelope_.IsIdle() &&
        envelopeSegment.firstValue <= kEnvelopeEpsilon &&
        envelopeSegment.step == 0.0f) {
      for (int remaining = processedSamples * channelCount;
           remaining < samplecount * channelCount; ++remaining) {
        output[remaining] = 0;
      }
      break;
    }

    AudioSample gain = baseGain * envelopeSegment.firstValue;
    const AudioSample gainStep = baseGain * envelopeSegment.step;
    const int segmentEnd = processedSamples + envelopeSegment.samples;
    for (; processedSamples < segmentEnd; ++processedSamples) {
      const int index = processedSamples * channelCount;
      output[index] = input[index] * gain;
      if (channelCount > 1) {
        output[index + 1] = input[index + 1] * gain;
      }
      gain += gainStep;
    }
  }
}

void AudioVoice::processPan(AudioSample *input, AudioSample *output,
                            int samplecount, AudioBufferLayout inputLayout,
                            AudioBufferLayout outputLayout) {
  (void)outputLayout;
  const int pan =
      clampPan(getVoiceValue(LFOTarget::VoicePan, params_.pan_, 0x7F, 0, 0xFF));

  const AudioSample left = panGainLeft(pan);
  const AudioSample right = panGainRight(pan);
  if (inputLayout == AudioBufferLayout::MonoPacked) {
    for (int i = 0; i < samplecount; ++i) {
      const AudioSample mono = input[i];
      const int index = i * 2;
      output[index] = mono * right;
      output[index + 1] = mono * left;
    }
    return;
  }

  for (int i = 0; i < samplecount; ++i) {
    const int index = i * 2;
    const AudioSample inputLeft = input[index];
    const AudioSample inputRight = input[index + 1];

    // Swap the logical stereo pair here to compensate for the reversed
    // hardware output wiring. This keeps the legacy sample path untouched
    // while every voiced instrument that flows through AudioVoice is fixed in
    // one place.
    output[index] = inputRight * right;
    output[index + 1] = inputLeft * left;
  }
}

void AudioVoice::processPostInsertStage(AudioSample *input, AudioSample *output,
                                        int samplecount,
                                        AudioBufferLayout inputLayout,
                                        AudioBufferLayout outputLayout) {
  (void)input;
  (void)outputLayout;
  processInsertChain(postInsertChain_, output, samplecount, inputLayout);
}

void AudioVoice::processInsertChain(etl::ivector<InsertEffectSlot *> &chain,
                                    AudioSample *buffer, int samplecount,
                                    AudioBufferLayout layout) {
  for (auto *slot : chain) {
    const int index = static_cast<int>(slot - &insertEffectSlots_[0]);
    Variable *amount =
        index == 0 ? params_.insertAmount1_ : params_.insertAmount2_;
    const LFOTarget target = index == 0 ? LFOTarget::VoiceInsertAmount1
                                        : LFOTarget::VoiceInsertAmount2;
    const int slotAmount =
        getVoiceValue(target, amount, kDefaultVoiceInsertAmount, 0, 0xFF);
    slot->Render(buffer, samplecount, slotAmount, layout);
  }
}

InsertEffectProcessor *AudioVoice::createInsertEffect(int insertType) {
  switch (ClampVoiceInsertEffectIndex(insertType)) {
  case kVoiceInsertEffectPassthrough:
    return insertEffectPool_.create<PassthroughInsertEffect>();
  case kVoiceInsertEffectOverdrive:
    return insertEffectPool_.create<OverdriveInsertEffect>();
  case kVoiceInsertEffectBitcrush:
    return insertEffectPool_.create<BitcrushInsertEffect>();
  case kVoiceInsertEffectDownsample:
    return insertEffectPool_.create<DownsampleInsertEffect>();
  case kVoiceInsertEffectSlew:
    return insertEffectPool_.create<SlewInsertEffect>();
  case kVoiceInsertEffectRectify:
    return insertEffectPool_.create<RectifyInsertEffect>();
  default:
    return nullptr;
  }
}

void AudioVoice::destroyInsertEffectSlot(int slot) {
  InsertEffectProcessor *effect = insertEffectSlots_[slot].GetEffect();
  if (!effect) {
    insertTypes_[slot] = kDefaultVoiceInsertEffectIndex;
    return;
  }

  switch (ClampVoiceInsertEffectIndex(insertTypes_[slot])) {
  case kVoiceInsertEffectPassthrough:
    insertEffectPool_.destroy(static_cast<PassthroughInsertEffect *>(effect));
    break;
  case kVoiceInsertEffectOverdrive:
    insertEffectPool_.destroy(static_cast<OverdriveInsertEffect *>(effect));
    break;
  case kVoiceInsertEffectBitcrush:
    insertEffectPool_.destroy(static_cast<BitcrushInsertEffect *>(effect));
    break;
  case kVoiceInsertEffectDownsample:
    insertEffectPool_.destroy(static_cast<DownsampleInsertEffect *>(effect));
    break;
  case kVoiceInsertEffectSlew:
    insertEffectPool_.destroy(static_cast<SlewInsertEffect *>(effect));
    break;
  case kVoiceInsertEffectRectify:
    insertEffectPool_.destroy(static_cast<RectifyInsertEffect *>(effect));
    break;
  default:
    break;
  }

  effect = nullptr;
  insertTypes_[slot] = kDefaultVoiceInsertEffectIndex;
}

void AudioVoice::clearInsertEffects() {
  preInsertChain_.clear();
  postInsertChain_.clear();
  for (int slot = 0; slot < InsertSlotCount; ++slot) {
    destroyInsertEffectSlot(slot);
    insertEffectSlots_[slot].SetEffect(
        createInsertEffect(kDefaultVoiceInsertEffectIndex));
    insertTypes_[slot] = kDefaultVoiceInsertEffectIndex;
    insertPositions_[slot] = kDefaultVoiceInsertPosition;
  }
  rebuildInsertChains();
}

void AudioVoice::syncInsertEffects() {
  const bool slot0Changed =
      syncInsertEffectSlot(0, params_.insertType1_, params_.insertPosition1_);
  const bool slot1Changed =
      syncInsertEffectSlot(1, params_.insertType2_, params_.insertPosition2_);
  if (slot0Changed || slot1Changed) {
    rebuildInsertChains();
    refreshStageLFOMasks();
  }
}

bool AudioVoice::syncInsertEffectSlot(int slot, Variable *type,
                                      Variable *position) {
  const int slotType = ClampVoiceInsertEffectIndex(
      type ? type->GetInt() : kDefaultVoiceInsertEffectIndex);
  const int slotPosition = ClampVoiceInsertPositionIndex(
      position ? position->GetInt() : kDefaultVoiceInsertPosition);
  bool changed = false;

  if (insertTypes_[slot] != slotType) {
    destroyInsertEffectSlot(slot);
    InsertEffectProcessor *effect = createInsertEffect(slotType);
    insertEffectSlots_[slot].SetEffect(effect);
    insertTypes_[slot] = slotType;
    insertEffectSlots_[slot].ResetState();
    changed = true;
  }

  if (insertPositions_[slot] != slotPosition) {
    insertEffectSlots_[slot].ResetState();
    changed = true;
  }

  insertPositions_[slot] = slotPosition;
  return changed;
}

void AudioVoice::rebuildInsertChains() {
  preInsertChain_.clear();
  postInsertChain_.clear();

  for (int slot = 0; slot < InsertSlotCount; ++slot) {
    if (insertPositions_[slot] == kVoiceInsertPositionPre) {
      preInsertChain_.push_back(&insertEffectSlots_[slot]);
    } else {
      postInsertChain_.push_back(&insertEffectSlots_[slot]);
    }
  }
}
