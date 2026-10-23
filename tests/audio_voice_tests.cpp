/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "doctest/doctest.h"

#include "Application/Instruments/I_Instrument.h"
#include "Application/Instruments/VoiceParameters.h"
#define private public
#include "Application/Player/AudioVoice.h"
#undef private
#include "Application/Audio/AudioSample.h"
#include "Application/Player/InsertEffectDefinitions.h"
#include "Application/Player/SyncMaster.h"
#include "Application/Player/VoiceADSR.h"
#include "Application/Utils/fixed.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

SubService::SubService(int fourCC, bool registerWithService)
    : fourCC_(fourCC), registerWithService_(registerWithService) {}

SubService::~SubService() = default;

Persistent::Persistent(const char *nodeName, bool registerWithService)
    : SubService(FourCC::ServicePersistency, false) {
  (void)nodeName;
  (void)registerWithService;
}

void Persistent::Save(tinyxml2::XMLPrinter *printer) { (void)printer; }

bool Persistent::Restore(PersistencyDocument *doc) {
  (void)doc;
  return false;
}

I_Instrument::~I_Instrument() = default;

void I_Instrument::SaveContent(tinyxml2::XMLPrinter *printer) { (void)printer; }

void I_Instrument::RestoreContent(PersistencyDocument *doc) { (void)doc; }

void I_Instrument::Purge() {
  for (auto it = Variables()->begin(); it != Variables()->end(); ++it) {
    (*it)->Reset();
  }
}

void TableSaveState::Reset() {
  position_[0] = 0;
  position_[1] = 0;
  position_[2] = 0;
  for (int i = 0; i < TABLE_STEPS; ++i) {
    hopCount_[i][0] = 0;
    hopCount_[i][1] = 0;
    hopCount_[i][2] = 0;
  }
}

namespace {
class StubInstrument : public I_Instrument {
public:
  StubInstrument()
      : I_Instrument(&variables_), voiceParameters_(), levelLeft_(0),
        levelRight_(0), active_(false), startResult_(true), alternating_(false),
        ramping_(false), stepLeft_(0.0f), stepRight_(0.0f), renderCount_(0),
        releaseCount_(0), layout_(AudioBufferLayout::StereoInterleaved),
        lastUpdateTick_(false) {
    voiceParameters_.AppendTo(variables_);
  }

  bool Init() override { return true; }

  bool Start(int channel, unsigned char note, bool retrigger) override {
    (void)channel;
    (void)note;
    lastRetrigger_ = retrigger;
    startCount_++;
    active_ = startResult_;
    return startResult_;
  }

  void Stop(int channel) override {
    (void)channel;
    stopCount_++;
    active_ = false;
  }

  void Release(int channel) override {
    (void)channel;
    releaseCount_++;
  }

  void OnStart() override {}

  bool Render(int channel, AudioSample *buffer, int size,
              bool updateTick) override {
    (void)channel;
    if (!active_) {
      return false;
    }

    lastUpdateTick_ = updateTick;
    for (int i = 0; i < size; ++i) {
      AudioSample left = levelLeft_;
      AudioSample right = levelRight_;
      if (ramping_) {
        left += stepLeft_ * static_cast<AudioSample>(renderCount_ + i);
        right += stepRight_ * static_cast<AudioSample>(renderCount_ + i);
      }
      if (alternating_ && ((renderCount_ + i) & 1)) {
        left = -left;
        right = -right;
      }
      if (layout_ == AudioBufferLayout::MonoPacked) {
        buffer[i] = left;
      } else {
        buffer[i * 2] = left;
        buffer[i * 2 + 1] = right;
      }
    }
    renderCount_ += size;
    return true;
  }

  AudioBufferLayout GetAudioBufferLayout(int channel) const override {
    (void)channel;
    return layout_;
  }

  bool IsInitialized() override { return true; }
  bool IsEmpty() override { return false; }
  InstrumentType GetType() override { return IT_SAMPLE_SOURCE; }
  void ProcessCommand(int channel, TrackerCommand cc, ushort value) override {
    (void)channel;
    (void)cc;
    (void)value;
  }
  int GetTable() override { return VAR_OFF; }
  bool GetTableAutomation() override { return false; }
  void GetTableState(TableSaveState &state) override { state.Reset(); }
  void SetTableState(TableSaveState &state) override { (void)state; }
  etl::ivector<Variable *> *Variables() override { return &variables_; }

  void SetSignal(AudioSample left, AudioSample right) {
    levelLeft_ = left;
    levelRight_ = right;
  }
  void SetAudioBufferLayout(AudioBufferLayout layout) { layout_ = layout; }

  void SetAlternating(bool alternating) { alternating_ = alternating; }
  void SetRamp(AudioSample stepLeft, AudioSample stepRight) {
    ramping_ = true;
    stepLeft_ = stepLeft;
    stepRight_ = stepRight;
  }

  Variable *VoiceVar(FourCC id) { return FindVariable(id); }
  LFOSettings *VoiceLFOSettings() { return voiceParameters_.GetLFOSettings(); }
  int ReleaseCount() const { return releaseCount_; }
  int StartCount() const { return startCount_; }
  int StopCount() const { return stopCount_; }
  bool LastRetrigger() const { return lastRetrigger_; }

  bool lastUpdateTick_;

private:
  static constexpr size_t kStubVariableCount =
      VoiceParameters::PersistedVariableCount;
  etl::vector<Variable *, kStubVariableCount> variables_;
  VoiceParameters voiceParameters_;
  AudioSample levelLeft_;
  AudioSample levelRight_;
  bool active_;
  bool startResult_;
  bool alternating_;
  bool ramping_;
  AudioSample stepLeft_;
  AudioSample stepRight_;
  int renderCount_;
  int releaseCount_;
  int startCount_ = 0;
  int stopCount_ = 0;
  bool lastRetrigger_ = true;
  AudioBufferLayout layout_;
};

void ConfigureLFO(StubInstrument &instrument, size_t slot, LFOTarget target,
                  LFOShape shape, int depth,
                  int rateValue = LFOSettings::DefaultRateValue) {
  LFOSettings *settings = instrument.VoiceLFOSettings();
  REQUIRE(settings != nullptr);
  settings->SetTarget(slot, target);
  settings->SetShape(slot, shape);
  settings->SetDepth(slot, depth);
  settings->SetRateValue(slot, rateValue);
}

template <size_t Size>
AudioSample sumAbs(const std::array<AudioSample, Size> &buffer) {
  AudioSample total = 0;
  for (AudioSample value : buffer) {
    total += std::abs(value);
  }
  return total;
}

constexpr AudioSample pcmToAudioSample(int pcm) {
  return static_cast<AudioSample>(pcm) / kAudioPcmScale;
}

AudioSample expectedOverdriveSample(AudioSample sample, int drive) {
  constexpr int kAmountMax = 0xFF;
  constexpr int kControlStepMax = 127;
  constexpr int kControlStepCount = 128;
  constexpr float kMinDrive = 0.15f;
  constexpr float kMaxDrive = 1.0f;
  constexpr float kDriveRange = kMaxDrive - kMinDrive;
  constexpr int kStageCount = 3;

  const int clampedDrive = std::clamp(drive, 0, kAmountMax);
  if (clampedDrive == 0) {
    return sample;
  }

  const int control =
      ((clampedDrive * kControlStepMax) + (kAmountMax / 2)) / kAmountMax;
  const float normalizedControl =
      static_cast<float>(control) / static_cast<float>(kControlStepCount);
  const float driveControl =
      kMinDrive + (kDriveRange * normalizedControl * normalizedControl);
  const AudioSample driven = sample * driveControl;
  const bool negative = driven < 0.0f;
  AudioSample shaped = negative ? -driven : driven;
  shaped = std::min(shaped, kAudioSampleMax);

  for (int i = 0; i < kStageCount; ++i) {
    shaped = (2.0f * shaped) - (shaped * shaped);
  }

  shaped = std::min(shaped, kAudioSampleMax);
  return negative ? -shaped : shaped;
}

AudioSample expectedBitcrushQuantizedSample(AudioSample sample, int amount) {
  constexpr int32_t kMinPcm = -32768;
  constexpr int32_t kMaxPcm = 32767;
  constexpr int kMaxBitReduction = 15;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int inverse = 0xFF - clampedAmount;
  const int curvedAmount = 0xFF - ((inverse * inverse + (0xFF / 2)) / 0xFF);
  const int shift = (curvedAmount * kMaxBitReduction + (0xFF / 2)) / 0xFF;
  const int32_t quantizationStep = 1 << shift;

  const AudioSample scaled = sample * kAudioPcmScale;
  const int32_t pcm = std::clamp(
      static_cast<int32_t>(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f),
      kMinPcm, kMaxPcm);
  if (pcm == 0) {
    return 0.0f;
  }
  if (quantizationStep == 1) {
    return static_cast<AudioSample>(pcm) / kAudioPcmScale;
  }

  const int32_t halfStep = quantizationStep >> 1;
  const bool negative = pcm < 0;
  const int32_t magnitude = std::min<int32_t>(negative ? -pcm : pcm, kMaxPcm);
  const int32_t quantizedMagnitude =
      ((magnitude / quantizationStep) * quantizationStep) + halfStep;
  const int32_t quantized = std::clamp(
      negative ? -quantizedMagnitude : quantizedMagnitude, kMinPcm, kMaxPcm);
  return static_cast<AudioSample>(quantized) / kAudioPcmScale;
}

AudioSample expectedClipSample(AudioSample sample, int amount) {
  constexpr int32_t kMaxPcm = 32767;
  constexpr int32_t kMinThreshold = 2048;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int32_t threshold =
      kMaxPcm -
      ((clampedAmount * (kMaxPcm - kMinThreshold)) + (0xFF / 2)) / 0xFF;
  const int32_t pcm = std::clamp(static_cast<int32_t>(sample * kAudioPcmScale),
                                 -kMaxPcm, kMaxPcm);
  return static_cast<AudioSample>(std::clamp(pcm, -threshold, threshold)) /
         kAudioPcmScale;
}

AudioSample expectedRectifySample(AudioSample sample, int amount) {
  constexpr int32_t kMinPcm = -32768;
  constexpr int32_t kMaxPcm = 32767;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int32_t pcm = std::clamp(static_cast<int32_t>(sample * kAudioPcmScale),
                                 kMinPcm, kMaxPcm);
  const int32_t rectified = std::min<int32_t>(pcm < 0 ? -pcm : pcm, kMaxPcm);
  const int32_t blended =
      pcm + ((clampedAmount * (rectified - pcm)) + (0xFF / 2)) / 0xFF;
  return static_cast<AudioSample>(blended) / kAudioPcmScale;
}

AudioSample expectedDCOffsetSample(AudioSample sample, int amount) {
  constexpr int32_t kMinPcm = -32768;
  constexpr int32_t kMaxPcm = 32767;
  constexpr int32_t kMaxOffset = 8192;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int32_t offset = ((clampedAmount - 0x80) * kMaxOffset) / 0x80;
  const int32_t pcm = std::clamp(static_cast<int32_t>(sample * kAudioPcmScale),
                                 kMinPcm, kMaxPcm);
  return static_cast<AudioSample>(std::clamp(pcm + offset, kMinPcm, kMaxPcm)) /
         kAudioPcmScale;
}

doctest::String effectShortName(const InsertEffectProcessor *effect) {
  return effect ? doctest::String(effect->GetShortName()) : doctest::String("");
}

AudioSample renderFilterTypeEnergy(int filterType, bool alternating) {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(16000), pcmToAudioSample(16000));
  instrument.SetAlternating(alternating);
  instrument.VoiceVar(FourCC::VoiceFilterCutoff)->SetInt(64, false);
  instrument.VoiceVar(FourCC::VoiceFilterResonance)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceFilterType)->SetInt(filterType, false);

  AudioVoice voice(0);
  if (!voice.StartInstrument(&instrument, 60, true)) {
    return 0.0f;
  }

  std::array<AudioSample, 256> buffer{};
  for (int i = 0; i < 32; ++i) {
    if (!voice.Render(buffer.data(), 128, false)) {
      return 0.0f;
    }
  }
  if (!voice.Render(buffer.data(), 128, false)) {
    return 0.0f;
  }
  return sumAbs(buffer);
}
} // namespace

TEST_CASE("AudioVoice open-path stays close to source audio after startup") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(16000));

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 8> warmup{};
  std::array<AudioSample, 8> buffer{};
  REQUIRE(voice.Render(warmup.data(), 4, true));
  CHECK(instrument.lastUpdateTick_);
  REQUIRE(voice.Render(buffer.data(), 4, false));

  for (int i = 0; i < 4; ++i) {
    CHECK(std::abs(buffer[i * 2] - pcmToAudioSample(16000)) <
          pcmToAudioSample(1000));
    CHECK(std::abs(buffer[i * 2 + 1] - pcmToAudioSample(12000)) <
          pcmToAudioSample(1200));
  }
}

TEST_CASE("AudioVoice volume command applies per-voice amp override") {
  SyncMaster::GetInstance()->SetTempo(120);

  StubInstrument instrument;
  instrument.SetSignal(0.5f, 0.5f);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 8> baseline{};
  REQUIRE(voice.Render(baseline.data(), 4, false));
  const AudioSample baselineEnergy = sumAbs(baseline);

  REQUIRE(
      voice.ProcessCommand(TrackerCommand::InstrumentCommandVolume, 0x0080));

  std::array<AudioSample, 8> commanded{};
  REQUIRE(voice.Render(commanded.data(), 4, false));
  const AudioSample commandedEnergy = sumAbs(commanded);

  CHECK(instrument.VoiceVar(FourCC::VoiceAmp)->GetInt() == 0xFF);
  CHECK(commandedEnergy < baselineEnergy * 0.7f);
  CHECK(commandedEnergy > baselineEnergy * 0.4f);
}

TEST_CASE("AudioVoice volume ramp advances only between render buffers") {
  SyncMaster::GetInstance()->SetTempo(120);

  StubInstrument instrument;
  instrument.SetSignal(0.5f, 0.5f);
  instrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceAmpAttack)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpDecay)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpSustain)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 48> baseline{};
  REQUIRE(voice.Render(baseline.data(), 24, false));
  const AudioSample baselineEnergy = sumAbs(baseline);

  REQUIRE(
      voice.ProcessCommand(TrackerCommand::InstrumentCommandVolume, 0x0100));
  CHECK(voice.ampCommand_.current_ == doctest::Approx(0xFF));
  CHECK(voice.ampCommand_.rampActive_);

  std::array<AudioSample, 48> firstBuffer{};
  REQUIRE(voice.Render(firstBuffer.data(), 24, false));
  const float afterFirstRender = voice.ampCommand_.current_;

  std::array<AudioSample, 48> secondBuffer{};
  REQUIRE(voice.Render(secondBuffer.data(), 24, false));

  CHECK(sumAbs(firstBuffer) == doctest::Approx(baselineEnergy).epsilon(0.01f));
  CHECK(afterFirstRender < 0xFF);
  CHECK(afterFirstRender > 0.0f);
  CHECK(voice.ampCommand_.current_ < afterFirstRender);
  CHECK(sumAbs(secondBuffer) < sumAbs(firstBuffer));
}

TEST_CASE("AudioVoice pan command applies per-voice pan override") {
  SyncMaster::GetInstance()->SetTempo(120);

  StubInstrument instrument;
  instrument.SetSignal(0.5f, 0.0f);
  instrument.SetAudioBufferLayout(AudioBufferLayout::MonoPacked);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  REQUIRE(voice.ProcessCommand(TrackerCommand::InstrumentCommandPan, 0x0000));

  std::array<AudioSample, 8> buffer{};
  REQUIRE(voice.Render(buffer.data(), 4, false));

  CHECK(instrument.VoiceVar(FourCC::VoicePan)->GetInt() == 0x7F);
  CHECK(sumAbs(buffer) > 0.0f);
  CHECK(std::abs(buffer[0]) < 0.01f);
  CHECK(std::abs(buffer[1]) > 0.3f);
}

TEST_CASE("AudioVoice filter commands apply per-voice cutoff overrides") {
  SyncMaster::GetInstance()->SetTempo(120);

  StubInstrument instrument;
  instrument.SetSignal(0.5f, 0.5f);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  REQUIRE(voice.ProcessCommand(TrackerCommand::InstrumentCommandLowPassFilter,
                               0x1030));
  CHECK(voice.filterCutoffCommand_.overrideActive_);
  CHECK(voice.filterCutoffCommand_.current_ == doctest::Approx(0x10));
  CHECK(voice.filterResonanceCommand_.overrideActive_);
  CHECK(voice.filterResonanceCommand_.current_ == doctest::Approx(0x30));
  CHECK(!voice.filterCutoffCommand_.rampActive_);
  CHECK(!voice.filterResonanceCommand_.rampActive_);

  REQUIRE(
      voice.ProcessCommand(TrackerCommand::InstrumentCommandFilterCut, 0x0008));
  REQUIRE(voice.ProcessCommand(TrackerCommand::InstrumentCommandFilterResonance,
                               0x0040));
  CHECK(voice.filterCutoffCommand_.current_ == doctest::Approx(0x08));
  CHECK(voice.filterResonanceCommand_.current_ == doctest::Approx(0x40));
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterCutoff)->GetInt() == 0xFF);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterResonance)->GetInt() == 0);
}

TEST_CASE("VoiceParameters expose the full editable voice block") {
  StubInstrument instrument;

  CHECK(instrument.VoiceVar(FourCC::VoiceOverdrive) == nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceBandpassCutoff) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceBandpassResonance) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceBandpassMix) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterCutoff) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterResonance) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterType) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterMix) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpAttack) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpDecay) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpSustain) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpRelease) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterAttack) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterDecay) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterSustain) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterRelease) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterEnvAmount) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmp) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoicePan) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertType) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertAmount) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertAmount2) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertPosition) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertPosition2) != nullptr);

  CHECK(instrument.VoiceVar(FourCC::VoiceBandpassCutoff)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceBandpassMix)->GetInt() == 0xFF);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterCutoff)->GetInt() == 0xFF);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterResonance)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterType)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpAttack)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpDecay)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpSustain)->GetInt() == 0xFF);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmpRelease)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceTriggerLength) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceTriggerLength)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterAttack)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterDecay)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterSustain)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterRelease)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceFilterEnvAmount)->GetInt() == 0);
  CHECK(instrument.VoiceVar(FourCC::VoiceAmp)->GetInt() == 0xFF);
  CHECK(instrument.VoiceVar(FourCC::VoicePan)->GetInt() == 0x7F);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertType)->GetInt() ==
        kDefaultVoiceInsertEffectIndex);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertType2) != nullptr);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertType2)->GetInt() ==
        kDefaultVoiceInsertEffectIndex);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertAmount)->GetInt() ==
        kDefaultVoiceInsertAmount);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertAmount2)->GetInt() ==
        kDefaultVoiceInsertAmount);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertPosition)->GetInt() ==
        kDefaultVoiceInsertPosition);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertPosition2)->GetInt() ==
        kDefaultVoiceInsertPosition);
  CHECK(
      doctest::String(
          instrument.VoiceVar(FourCC::VoiceInsertType)->GetString().c_str()) ==
      doctest::String("none"));
  CHECK(
      doctest::String(
          instrument.VoiceVar(FourCC::VoiceInsertType2)->GetString().c_str()) ==
      doctest::String("none"));
  CHECK(doctest::String(instrument.VoiceVar(FourCC::VoiceInsertPosition)
                            ->GetString()
                            .c_str()) == doctest::String("pre"));
  CHECK(doctest::String(instrument.VoiceVar(FourCC::VoiceInsertPosition2)
                            ->GetString()
                            .c_str()) == doctest::String("pre"));
}

TEST_CASE(
    "VoiceParameters changing insert type resets amount but keeps position") {
  StubInstrument instrument;

  instrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0x73, false);
  instrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  instrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectBitcrush, true);

  CHECK(instrument.VoiceVar(FourCC::VoiceInsertAmount2)->GetInt() ==
        kDefaultVoiceInsertAmount);
  CHECK(instrument.VoiceVar(FourCC::VoiceInsertPosition2)->GetInt() ==
        kVoiceInsertPositionPost);
}

TEST_CASE("VoiceParameters restore persisted insert type names to UI labels") {
  StubInstrument instrument;

  struct PersistNameCase {
    const char *name;
    int32_t expected;
  };

  const PersistNameCase persistNames[] = {
      {"none", kVoiceInsertEffectPassthrough},
      {"overdrive", kVoiceInsertEffectOverdrive},
      {"bitcrush", kVoiceInsertEffectBitcrush},
      {"downsample", kVoiceInsertEffectDownsample},
      {"rectify", kVoiceInsertEffectRectify},
  };

  for (const auto &persistName : persistNames) {
    instrument.VoiceVar(FourCC::VoiceInsertType)
        ->SetString(persistName.name, false);
    CHECK(instrument.VoiceVar(FourCC::VoiceInsertType)->GetInt() ==
          persistName.expected);
    CHECK(doctest::String(instrument.VoiceVar(FourCC::VoiceInsertType)
                              ->GetString()
                              .c_str()) ==
          doctest::String(
              GetVoiceInsertEffectInfo(persistName.expected).displayName_));
  }
}

TEST_CASE("AudioVoice insert bitcrush quantizes the rendered signal") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(0.1234f, -0.1234f);

  StubInstrument crushInstrument;
  crushInstrument.SetSignal(0.1234f, -0.1234f);
  crushInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  crushInstrument.VoiceVar(FourCC::VoiceInsertAmount)->SetInt(0xFF, false);
  crushInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice crushVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(crushVoice.StartInstrument(&crushInstrument, 60, true));

  std::array<AudioSample, 4> dryWarmup{};
  std::array<AudioSample, 4> crushWarmup{};
  REQUIRE(dryVoice.Render(dryWarmup.data(), 2, false));
  REQUIRE(crushVoice.Render(crushWarmup.data(), 2, false));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> crushBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(crushVoice.Render(crushBuffer.data(), 2, false));

  CHECK(crushBuffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[0], 0xFF)));
  CHECK(crushBuffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[1], 0xFF)));
  CHECK(crushBuffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[2], 0xFF)));
  CHECK(crushBuffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice second insert slot resolves and applies bitcrush") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(0.1234f, -0.1234f);

  StubInstrument crushInstrument;
  crushInstrument.SetSignal(0.1234f, -0.1234f);
  crushInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  crushInstrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xFF, false);
  crushInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice crushVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(crushVoice.StartInstrument(&crushInstrument, 60, true));

  CHECK(effectShortName(crushVoice.insertEffectSlots_[0].GetEffect()) ==
        doctest::String("PAS"));
  CHECK(effectShortName(crushVoice.insertEffectSlots_[1].GetEffect()) ==
        doctest::String("CRS"));

  std::array<AudioSample, 4> dryWarmup{};
  std::array<AudioSample, 4> crushWarmup{};
  REQUIRE(dryVoice.Render(dryWarmup.data(), 2, false));
  REQUIRE(crushVoice.Render(crushWarmup.data(), 2, false));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> crushBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(crushVoice.Render(crushBuffer.data(), 2, false));

  CHECK(crushBuffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[0], 0xFF)));
  CHECK(crushBuffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[1], 0xFF)));
  CHECK(crushBuffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[2], 0xFF)));
  CHECK(crushBuffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice updates second insert slot selection without retrigger") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(0.1234f, -0.1234f);
  StubInstrument crushInstrument;
  crushInstrument.SetSignal(0.1234f, -0.1234f);

  AudioVoice dryVoice(0);
  AudioVoice crushVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(crushVoice.StartInstrument(&crushInstrument, 60, true));
  CHECK(effectShortName(crushVoice.insertEffectSlots_[1].GetEffect()) ==
        doctest::String("PAS"));

  std::array<AudioSample, 4> warmup{};
  std::array<AudioSample, 4> dryWarmup{};
  REQUIRE(dryVoice.Render(dryWarmup.data(), 2, false));
  REQUIRE(crushVoice.Render(warmup.data(), 2, false));

  crushInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  crushInstrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xFF, false);
  crushInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> crushBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(crushVoice.Render(crushBuffer.data(), 2, false));

  CHECK(effectShortName(crushVoice.insertEffectSlots_[1].GetEffect()) ==
        doctest::String("CRS"));
  CHECK(crushBuffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[0], 0xFF)));
  CHECK(crushBuffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[1], 0xFF)));
  CHECK(crushBuffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[2], 0xFF)));
  CHECK(crushBuffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(dryBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice updates insert amount without retrigger") {
  StubInstrument instrument;
  instrument.SetSignal(0.5f, -0.5f);
  instrument.SetAlternating(true);
  instrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  instrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 4> lowAmountBuffer{};
  REQUIRE(voice.Render(lowAmountBuffer.data(), 2, false));

  instrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xC0, false);

  std::array<AudioSample, 4> highAmountBuffer{};
  REQUIRE(voice.Render(highAmountBuffer.data(), 2, false));

  CHECK(highAmountBuffer[2] != doctest::Approx(highAmountBuffer[0]));
  CHECK(highAmountBuffer[3] != doctest::Approx(highAmountBuffer[1]));
  CHECK(lowAmountBuffer[2] != doctest::Approx(lowAmountBuffer[0]));
  CHECK(highAmountBuffer[0] != doctest::Approx(lowAmountBuffer[0]));
  CHECK(highAmountBuffer[1] != doctest::Approx(lowAmountBuffer[1]));
}

TEST_CASE("AudioVoice second insert slot resolves and applies downsample") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(0.5f, -0.5f);
  dryInstrument.SetAlternating(true);

  StubInstrument effectedInstrument;
  effectedInstrument.SetSignal(0.5f, -0.5f);
  effectedInstrument.SetAlternating(true);
  effectedInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectDownsample, false);
  effectedInstrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xFF, false);
  effectedInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice effectedVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(effectedVoice.StartInstrument(&effectedInstrument, 60, true));

  CHECK(effectShortName(effectedVoice.insertEffectSlots_[1].GetEffect()) ==
        doctest::String("DSP"));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> effectedBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(effectedVoice.Render(effectedBuffer.data(), 2, false));

  CHECK(dryBuffer[2] != doctest::Approx(dryBuffer[0]));
  CHECK(dryBuffer[3] != doctest::Approx(dryBuffer[1]));
  CHECK(effectedBuffer[2] == doctest::Approx(effectedBuffer[0]));
  CHECK(effectedBuffer[3] == doctest::Approx(effectedBuffer[1]));
}

TEST_CASE("AudioVoice overdrive insert matches the three-stage curve") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(-15000));

  StubInstrument driveInstrument;
  driveInstrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(-15000));
  driveInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectOverdrive, false);
  driveInstrument.VoiceVar(FourCC::VoiceInsertAmount)->SetInt(0xFF, false);
  driveInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice driveVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(driveVoice.StartInstrument(&driveInstrument, 60, true));

  CHECK(effectShortName(driveVoice.insertEffectSlots_[0].GetEffect()) ==
        doctest::String("DRV"));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> driveBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(driveVoice.Render(driveBuffer.data(), 2, false));

  CHECK(driveBuffer[0] ==
        doctest::Approx(expectedOverdriveSample(dryBuffer[0], 0xFF)));
  CHECK(driveBuffer[1] ==
        doctest::Approx(expectedOverdriveSample(dryBuffer[1], 0xFF)));
  CHECK(driveBuffer[2] ==
        doctest::Approx(expectedOverdriveSample(dryBuffer[2], 0xFF)));
  CHECK(driveBuffer[3] ==
        doctest::Approx(expectedOverdriveSample(dryBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice second insert slot resolves and applies overdrive") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));

  StubInstrument driveInstrument;
  driveInstrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  driveInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectOverdrive, false);
  driveInstrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xFF, false);
  driveInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice driveVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(driveVoice.StartInstrument(&driveInstrument, 60, true));

  CHECK(effectShortName(driveVoice.insertEffectSlots_[1].GetEffect()) ==
        doctest::String("DRV"));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> driveBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(driveVoice.Render(driveBuffer.data(), 2, false));

  CHECK(std::abs(driveBuffer[0]) > std::abs(dryBuffer[0]));
  CHECK(std::abs(driveBuffer[1]) > std::abs(dryBuffer[1]));
  CHECK(std::abs(driveBuffer[0]) <= kAudioSampleMax);
  CHECK(std::abs(driveBuffer[1]) <= kAudioSampleMax);
}

TEST_CASE("AudioVoice slew insert limits fast steps") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(1.0f, -1.0f);
  dryInstrument.SetAlternating(true);

  StubInstrument slewInstrument;
  slewInstrument.SetSignal(1.0f, -1.0f);
  slewInstrument.SetAlternating(true);
  slewInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectSlew, false);
  slewInstrument.VoiceVar(FourCC::VoiceInsertAmount)->SetInt(0xFF, false);
  slewInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice slewVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(slewVoice.StartInstrument(&slewInstrument, 60, true));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> slewBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(slewVoice.Render(slewBuffer.data(), 2, false));

  CHECK(slewBuffer[0] == doctest::Approx(dryBuffer[0]).epsilon(0.001f));
  CHECK(slewBuffer[1] == doctest::Approx(dryBuffer[1]).epsilon(0.001f));
  CHECK(std::abs(slewBuffer[2] - slewBuffer[0]) <
        std::abs(dryBuffer[2] - dryBuffer[0]));
  CHECK(std::abs(slewBuffer[3] - slewBuffer[1]) <
        std::abs(dryBuffer[3] - dryBuffer[1]));
}

TEST_CASE("AudioVoice rectify insert flips the negative half-cycle") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(-0.6f, 0.25f);

  StubInstrument rectifyInstrument;
  rectifyInstrument.SetSignal(-0.6f, 0.25f);
  rectifyInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectRectify, false);
  rectifyInstrument.VoiceVar(FourCC::VoiceInsertAmount)->SetInt(0xFF, false);
  rectifyInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice rectifyVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(rectifyVoice.StartInstrument(&rectifyInstrument, 60, true));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> rectifyBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(rectifyVoice.Render(rectifyBuffer.data(), 2, false));

  CHECK(rectifyBuffer[0] ==
        doctest::Approx(expectedRectifySample(dryBuffer[0], 0xFF)));
  CHECK(rectifyBuffer[1] ==
        doctest::Approx(expectedRectifySample(dryBuffer[1], 0xFF)));
  CHECK(rectifyBuffer[2] ==
        doctest::Approx(expectedRectifySample(dryBuffer[2], 0xFF)));
  CHECK(rectifyBuffer[3] ==
        doctest::Approx(expectedRectifySample(dryBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice second insert slot resolves and applies rectify") {
  StubInstrument dryInstrument;
  dryInstrument.SetSignal(-0.25f, 0.125f);

  StubInstrument rectifiedInstrument;
  rectifiedInstrument.SetSignal(-0.25f, 0.125f);
  rectifiedInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectRectify, false);
  rectifiedInstrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xFF, false);
  rectifiedInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice dryVoice(0);
  AudioVoice rectifiedVoice(1);
  REQUIRE(dryVoice.StartInstrument(&dryInstrument, 60, true));
  REQUIRE(rectifiedVoice.StartInstrument(&rectifiedInstrument, 60, true));

  CHECK(effectShortName(rectifiedVoice.insertEffectSlots_[1].GetEffect()) ==
        doctest::String("RCT"));

  std::array<AudioSample, 4> dryBuffer{};
  std::array<AudioSample, 4> rectifiedBuffer{};
  REQUIRE(dryVoice.Render(dryBuffer.data(), 2, false));
  REQUIRE(rectifiedVoice.Render(rectifiedBuffer.data(), 2, false));

  CHECK(rectifiedBuffer[0] ==
        doctest::Approx(expectedRectifySample(dryBuffer[0], 0xFF)));
  CHECK(rectifiedBuffer[1] ==
        doctest::Approx(expectedRectifySample(dryBuffer[1], 0xFF)));
  CHECK(rectifiedBuffer[2] ==
        doctest::Approx(expectedRectifySample(dryBuffer[2], 0xFF)));
  CHECK(rectifiedBuffer[3] ==
        doctest::Approx(expectedRectifySample(dryBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice pre insert runs before the filter stages") {
  StubInstrument preInstrument;
  preInstrument.SetSignal(1.0f, 1.0f);
  preInstrument.SetAlternating(true);
  preInstrument.VoiceVar(FourCC::VoiceFilterCutoff)->SetInt(64, false);
  preInstrument.VoiceVar(FourCC::VoiceFilterResonance)->SetInt(0, false);
  preInstrument.VoiceVar(FourCC::VoiceFilterType)->SetInt(0, false);
  preInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectDownsample, false);
  preInstrument.VoiceVar(FourCC::VoiceInsertAmount)->SetInt(0xFF, false);
  preInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPre, false);

  StubInstrument postInstrument;
  postInstrument.SetSignal(1.0f, 1.0f);
  postInstrument.SetAlternating(true);
  postInstrument.VoiceVar(FourCC::VoiceFilterCutoff)->SetInt(64, false);
  postInstrument.VoiceVar(FourCC::VoiceFilterResonance)->SetInt(0, false);
  postInstrument.VoiceVar(FourCC::VoiceFilterType)->SetInt(0, false);
  postInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectDownsample, false);
  postInstrument.VoiceVar(FourCC::VoiceInsertAmount)->SetInt(0xFF, false);
  postInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);

  AudioVoice preVoice(0);
  AudioVoice postVoice(1);
  REQUIRE(preVoice.StartInstrument(&preInstrument, 60, true));
  REQUIRE(postVoice.StartInstrument(&postInstrument, 60, true));

  std::array<AudioSample, 4> preBuffer{};
  std::array<AudioSample, 4> postBuffer{};
  REQUIRE(preVoice.Render(preBuffer.data(), 2, false));
  REQUIRE(postVoice.Render(postBuffer.data(), 2, false));

  CHECK(preBuffer[2] != doctest::Approx(preBuffer[0]));
  CHECK(preBuffer[3] != doctest::Approx(preBuffer[1]));
  CHECK(postBuffer[2] == doctest::Approx(postBuffer[0]));
  CHECK(postBuffer[3] == doctest::Approx(postBuffer[1]));
}

TEST_CASE("AudioVoice updates insert position without retrigger") {
  StubInstrument instrument;
  instrument.SetSignal(0.25f, 0.25f);
  instrument.SetAlternating(true);
  instrument.VoiceVar(FourCC::VoiceBandpassCutoff)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectRectify, false);
  instrument.VoiceVar(FourCC::VoiceInsertAmount2)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPre, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 128> warmup{};
  REQUIRE(voice.Render(warmup.data(), 64, false));

  std::array<AudioSample, 16> preBuffer{};
  REQUIRE(voice.Render(preBuffer.data(), 8, false));

  instrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);

  std::array<AudioSample, 16> postBuffer{};
  REQUIRE(voice.Render(postBuffer.data(), 8, false));

  CHECK(voice.insertPositions_[1] == kVoiceInsertPositionPost);
  CHECK(sumAbs(postBuffer) > (sumAbs(preBuffer) * 8.0f));
}

TEST_CASE("LFO settings keep only one active slot per target") {
  StubInstrument instrument;

  REQUIRE(instrument.VoiceLFOSettings() != nullptr);
  instrument.VoiceLFOSettings()->SetTarget(0, LFOTarget::VoiceInsertAmount1);
  instrument.VoiceLFOSettings()->SetTarget(1, LFOTarget::VoiceInsertAmount1);

  CHECK(instrument.VoiceLFOSettings()->Get(0).target_ == LFOTarget::None);
  CHECK(instrument.VoiceLFOSettings()->Get(1).target_ ==
        LFOTarget::VoiceInsertAmount1);
}

TEST_CASE("LFO settings reject invalid targets") {
  StubInstrument instrument;

  REQUIRE(instrument.VoiceLFOSettings() != nullptr);
  instrument.VoiceLFOSettings()->SetTarget(0, static_cast<LFOTarget>(0x7F));
  instrument.VoiceLFOSettings()->SetTarget(1, static_cast<LFOTarget>(0xFF));

  CHECK(instrument.VoiceLFOSettings()->Get(0).target_ == LFOTarget::None);
  CHECK(instrument.VoiceLFOSettings()->Get(1).target_ == LFOTarget::None);
}

TEST_CASE("LFO settings clamp enum and rate values in setters") {
  StubInstrument instrument;

  REQUIRE(instrument.VoiceLFOSettings() != nullptr);
  instrument.VoiceLFOSettings()->SetShape(0, static_cast<LFOShape>(0x7F));
  instrument.VoiceLFOSettings()->SetTriggerMode(
      0, static_cast<LFOTriggerMode>(0x7F));
  instrument.VoiceLFOSettings()->SetRateMode(0, static_cast<LFORateMode>(0x7F));
  instrument.VoiceLFOSettings()->SetDepth(0, -5);
  instrument.VoiceLFOSettings()->SetRateValue(0, -5);
  instrument.VoiceLFOSettings()->SetSyncIndex(0, 999);
  instrument.VoiceLFOSettings()->SetRateValue(1, 999);

  const LFOConfig config = instrument.VoiceLFOSettings()->Get(0);
  const LFOConfig clampedMax = instrument.VoiceLFOSettings()->Get(1);
  CHECK(config.shape_ == LFOShape::Sine);
  CHECK(config.triggerMode_ == LFOTriggerMode::Trigger);
  CHECK(config.rateMode_ == LFORateMode::Hertz);
  CHECK(config.depth_ == 0);
  CHECK(config.rate_ == LFOSettings::MinRateValue);
  CHECK(clampedMax.rate_ == LFOSettings::MaxRateValue);
  CHECK(config.syncIndex_ == static_cast<int>(GetLFOSyncDivisionCount()) - 1);
}

TEST_CASE(
    "AudioVoice free-run LFO keeps phase across same-instrument retrigger") {
  StubInstrument instrument;
  ConfigureLFO(instrument, 0, LFOTarget::VoiceFilterCutoff, LFOShape::Sine,
               0xFF, 0x40);
  instrument.VoiceLFOSettings()->SetTriggerMode(0, LFOTriggerMode::FreeRun);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  constexpr float runningPhase = 0.25f;
  voice.lfoBank_.states_[0].phase_ = runningPhase;

  REQUIRE(voice.StartInstrument(&instrument, 64, true));

  CHECK(voice.lfoBank_.states_[0].phase_ == doctest::Approx(runningPhase));
  CHECK(voice.lfoBank_.states_[0].currentValue_ == doctest::Approx(1.0f));
}

TEST_CASE(
    "AudioVoice triggered LFO resets phase across same-instrument retrigger") {
  StubInstrument instrument;
  ConfigureLFO(instrument, 0, LFOTarget::VoiceFilterCutoff, LFOShape::Sine,
               0xFF, 0x40);
  instrument.VoiceLFOSettings()->SetTriggerMode(0, LFOTriggerMode::Trigger);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  voice.lfoBank_.states_[0].phase_ = 0.25f;

  REQUIRE(voice.StartInstrument(&instrument, 64, true));

  CHECK(voice.lfoBank_.states_[0].phase_ == doctest::Approx(0.0f));
  CHECK(voice.lfoBank_.states_[0].currentValue_ == doctest::Approx(0.0f));
}

TEST_CASE("VoiceADSR immediate note-on reaches sustain when stages are zero") {
  VoiceADSR env;
  env.SetParameters(0, 0, 0xFF, 0);
  env.NoteOn();

  CHECK(env.Advance() == doctest::Approx(1.0f));
  CHECK(env.Advance() == doctest::Approx(1.0f));
  CHECK(!env.IsIdle());
}

TEST_CASE("VoiceADSR attack ramps up and retrigger restarts from zero") {
  VoiceADSR env;
  env.SetParameters(64, 0, 0xFF, 0);
  env.NoteOn();

  const float first = env.Advance();
  const float second = env.Advance();
  const float third = env.Advance();
  CHECK(first > 0.0f);
  CHECK(first < 1.0f);
  CHECK(second > first);
  CHECK(third > second);

  env.NoteOn();
  const float retriggered = env.Advance();
  CHECK(retriggered > 0.0f);
  CHECK(retriggered < third);
}

TEST_CASE("VoiceADSR decay reaches sustain and release reaches idle") {
  VoiceADSR env;
  env.SetParameters(0, 64, 0x80, 64);
  env.NoteOn();

  float value = 0.0f;
  for (int i = 0; i < 20000; ++i) {
    value = env.Advance();
    if (value <= ((0x80 / 255.0f) + 0.01f)) {
      break;
    }
  }
  CHECK(value == doctest::Approx(0x80 / 255.0f).epsilon(0.05f));

  env.NoteOff();
  for (int i = 0; i < 50000; ++i) {
    value = env.Advance();
    if (env.IsIdle()) {
      break;
    }
  }
  CHECK(env.IsIdle());
  CHECK(value == doctest::Approx(0.0f).epsilon(0.001f));
}

TEST_CASE("VoiceADSR decay slows as it approaches the target") {
  VoiceADSR env;
  env.SetParameters(0, 64, 0x00, 0);
  env.NoteOn();

  const float start = env.GetValue();
  env.AdvanceSilent(32);
  const float afterFirstBlock = env.GetValue();
  env.AdvanceSilent(32);
  const float afterSecondBlock = env.GetValue();

  const float firstDrop = start - afterFirstBlock;
  const float secondDrop = afterFirstBlock - afterSecondBlock;
  CHECK(firstDrop > 0.0f);
  CHECK(secondDrop > 0.0f);
  CHECK(firstDrop > secondDrop);
}

TEST_CASE("VoiceADSR maximum release holds until killed") {
  VoiceADSR env;
  env.SetParameters(0, 0, 0xFF, 0xFF);
  env.NoteOn();
  CHECK(env.Advance() == doctest::Approx(1.0f));

  env.NoteOff();
  env.AdvanceSilent(256);

  CHECK(!env.IsIdle());
  CHECK(env.GetValue() == doctest::Approx(1.0f));

  env.Kill();
  CHECK(env.IsIdle());
  CHECK(env.GetValue() == doctest::Approx(0.0f));
}

TEST_CASE("VoiceADSR release idles on a control segment boundary") {
  VoiceADSR env;
  env.SetParameters(0, 0, 0xFF, 1);
  env.NoteOn();
  REQUIRE(env.Advance() == doctest::Approx(1.0f));

  env.NoteOff();

  constexpr float idleThreshold = 1.0f / 65536.0f;
  int belowThresholdSample = -1;
  int idleSample = -1;
  for (int i = 1; i <= 1024; ++i) {
    const float value = env.Advance();
    if (belowThresholdSample < 0 && !env.IsIdle() && value <= idleThreshold) {
      belowThresholdSample = i;
    }
    if (env.IsIdle()) {
      idleSample = i;
      break;
    }
  }

  REQUIRE(belowThresholdSample > 0);
  REQUIRE(idleSample > belowThresholdSample);
  CHECK(idleSample - belowThresholdSample <= 32);
}

TEST_CASE("VoiceADSR block advance matches per-sample output") {
  VoiceADSR scalar;
  VoiceADSR blocked;
  scalar.SetParameters(4, 1, 0x40, 1);
  blocked.SetParameters(4, 1, 0x40, 1);
  scalar.NoteOn();
  blocked.NoteOn();

  const auto compare = [](VoiceADSR &single, VoiceADSR &chunked, int samples) {
    int processed = 0;
    while (processed < samples) {
      const int requested =
          std::min(samples - processed, 1 + ((processed * 7) % 41));
      const VoiceADSR::LinearSegment segment = chunked.AdvanceBlock(requested);

      REQUIRE(segment.samples > 0);
      REQUIRE(segment.samples <= requested);

      float value = segment.firstValue;
      for (int i = 0; i < segment.samples; ++i) {
        CHECK(value == doctest::Approx(single.Advance()).epsilon(0.0001f));
        value += segment.step;
      }

      processed += segment.samples;
      CHECK(chunked.GetValue() ==
            doctest::Approx(single.GetValue()).epsilon(0.0001f));
      CHECK(chunked.IsIdle() == single.IsIdle());
    }
  };

  compare(scalar, blocked, 768);
  scalar.NoteOff();
  blocked.NoteOff();
  compare(scalar, blocked, 768);
}

TEST_CASE("AudioVoice legato start keeps the running envelope alive") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(16000), pcmToAudioSample(16000));
  instrument.VoiceVar(FourCC::VoiceAmpAttack)->SetInt(64, false);
  instrument.VoiceVar(FourCC::VoiceAmpDecay)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpSustain)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 2> firstBuffer{};
  std::array<AudioSample, 2> secondBuffer{};
  std::array<AudioSample, 2> legatoBuffer{};
  REQUIRE(voice.Render(firstBuffer.data(), 1, false));
  REQUIRE(voice.Render(secondBuffer.data(), 1, false));

  const AudioSample levelBeforeLegato = std::abs(secondBuffer[0]);
  REQUIRE(voice.StartInstrument(&instrument, 67, false));
  CHECK(instrument.StartCount() == 2);
  CHECK(instrument.StopCount() == 0);
  CHECK(instrument.LastRetrigger() == false);

  REQUIRE(voice.Render(legatoBuffer.data(), 1, false));
  CHECK(std::abs(legatoBuffer[0]) > levelBeforeLegato);
}

TEST_CASE("VoiceADSR decay follows live sustain changes") {
  VoiceADSR env;
  env.SetParameters(0, 64, 0x20, 0);
  env.NoteOn();

  float value = 1.0f;
  for (int i = 0; i < 4000; ++i) {
    value = env.Advance();
    if (value < 0.4f) {
      break;
    }
  }

  env.SetParameters(0, 64, 0xF0, 0);
  const float first = env.Advance();
  const float second = env.Advance();

  CHECK(first > value);
  CHECK(second > first);
}

TEST_CASE("VoiceADSR release responds to live release changes") {
  VoiceADSR env;
  env.SetParameters(0, 0, 0xFF, 64);
  env.NoteOn();
  CHECK(env.Advance() == doctest::Approx(1.0f));

  env.NoteOff();
  CHECK(env.Advance() < 1.0f);

  env.SetParameters(0, 0, 0xFF, 0);
  CHECK(env.IsIdle());
  CHECK(env.GetValue() == doctest::Approx(0.0f));
}

TEST_CASE("VoiceADSR kill clears the envelope immediately") {
  VoiceADSR env;
  env.SetParameters(64, 64, 0x80, 64);
  env.NoteOn();
  CHECK(env.Advance() > 0.0f);

  env.Kill();

  CHECK(env.IsIdle());
  CHECK(env.GetValue() == doctest::Approx(0.0f));
  CHECK(env.Advance() == doctest::Approx(0.0f));
}

TEST_CASE("AudioVoice amp and pan scale the rendered signal") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0x80, false);
  instrument.VoiceVar(FourCC::VoicePan)->SetInt(0, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 4> buffer{};
  REQUIRE(voice.Render(buffer.data(), 2, false));

  // Engine channel order is right, left. Hard-left pan must therefore mute the
  // first sample in each pair and keep the second one audible.
  CHECK(buffer[0] == 0);
  CHECK(buffer[1] > 0);
  CHECK(buffer[2] == 0);
  CHECK(buffer[3] > 0);
  CHECK(buffer[1] < pcmToAudioSample(12000));
}

TEST_CASE("AudioVoice center pan swaps stereo output for voiced instruments") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(4000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoicePan)->SetInt(0x7F, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 2> warmup{};
  std::array<AudioSample, 2> buffer{};
  REQUIRE(voice.Render(warmup.data(), 1, true));
  REQUIRE(voice.Render(buffer.data(), 1, false));

  CHECK(std::abs(buffer[0]) > std::abs(buffer[1]));
  CHECK(buffer[0] == doctest::Approx(buffer[1] * 3.0f).epsilon(0.01f));
}

TEST_CASE("AudioVoice packed mono expands to stereo only at pan") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), 0.0f);
  instrument.SetAudioBufferLayout(AudioBufferLayout::MonoPacked);
  instrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoicePan)->SetInt(0x7F, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 2> buffer{};
  REQUIRE(voice.Render(buffer.data(), 1, false));

  CHECK(buffer[0] == doctest::Approx(buffer[1]).epsilon(0.01f));
  CHECK(buffer[0] > 0.0f);
}

TEST_CASE("AudioVoice applies live LFO values to continuous targets") {
  StubInstrument baseInstrument;
  StubInstrument modInstrument;
  baseInstrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  modInstrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  baseInstrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0x80, false);
  modInstrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0x80, false);
  ConfigureLFO(modInstrument, 0, LFOTarget::VoiceAmp, LFOShape::Ramp, 96);

  AudioVoice baseVoice(0);
  AudioVoice modVoice(1);
  REQUIRE(baseVoice.StartInstrument(&baseInstrument, 60, true));
  REQUIRE(modVoice.StartInstrument(&modInstrument, 60, true));

  std::array<AudioSample, 8> baseBuffer{};
  std::array<AudioSample, 8> modBuffer{};
  REQUIRE(baseVoice.Render(baseBuffer.data(), 4, false));
  REQUIRE(modVoice.Render(modBuffer.data(), 4, false));

  CHECK(sumAbs(modBuffer) < sumAbs(baseBuffer));
}

TEST_CASE("AudioVoice applies live LFO values to first insert amount") {
  StubInstrument baseInstrument;
  StubInstrument modInstrument;
  baseInstrument.SetSignal(0.1234f, -0.1234f);
  modInstrument.SetSignal(0.1234f, -0.1234f);
  baseInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  baseInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);
  modInstrument.VoiceVar(FourCC::VoiceInsertType)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  modInstrument.VoiceVar(FourCC::VoiceInsertPosition)
      ->SetInt(kVoiceInsertPositionPost, false);
  ConfigureLFO(modInstrument, 0, LFOTarget::VoiceInsertAmount1,
               LFOShape::Square, 0xFF);

  AudioVoice baseVoice(0);
  AudioVoice modVoice(1);
  REQUIRE(baseVoice.StartInstrument(&baseInstrument, 60, true));
  REQUIRE(modVoice.StartInstrument(&modInstrument, 60, true));

  std::array<AudioSample, 4> baseWarmup{};
  std::array<AudioSample, 4> modWarmup{};
  REQUIRE(baseVoice.Render(baseWarmup.data(), 2, false));
  REQUIRE(modVoice.Render(modWarmup.data(), 2, false));

  std::array<AudioSample, 4> baseBuffer{};
  std::array<AudioSample, 4> modBuffer{};
  REQUIRE(baseVoice.Render(baseBuffer.data(), 2, false));
  REQUIRE(modVoice.Render(modBuffer.data(), 2, false));

  CHECK(modBuffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[0], 0xFF)));
  CHECK(modBuffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[1], 0xFF)));
  CHECK(modBuffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[2], 0xFF)));
  CHECK(modBuffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice applies live LFO values to second insert amount") {
  StubInstrument baseInstrument;
  StubInstrument modInstrument;
  baseInstrument.SetSignal(0.1234f, -0.1234f);
  modInstrument.SetSignal(0.1234f, -0.1234f);
  baseInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  baseInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);
  modInstrument.VoiceVar(FourCC::VoiceInsertType2)
      ->SetInt(kVoiceInsertEffectBitcrush, false);
  modInstrument.VoiceVar(FourCC::VoiceInsertPosition2)
      ->SetInt(kVoiceInsertPositionPost, false);
  ConfigureLFO(modInstrument, 0, LFOTarget::VoiceInsertAmount2,
               LFOShape::Square, 0xFF);

  AudioVoice baseVoice(0);
  AudioVoice modVoice(1);
  REQUIRE(baseVoice.StartInstrument(&baseInstrument, 60, true));
  REQUIRE(modVoice.StartInstrument(&modInstrument, 60, true));

  std::array<AudioSample, 4> baseWarmup{};
  std::array<AudioSample, 4> modWarmup{};
  REQUIRE(baseVoice.Render(baseWarmup.data(), 2, false));
  REQUIRE(modVoice.Render(modWarmup.data(), 2, false));

  std::array<AudioSample, 4> baseBuffer{};
  std::array<AudioSample, 4> modBuffer{};
  REQUIRE(baseVoice.Render(baseBuffer.data(), 2, false));
  REQUIRE(modVoice.Render(modBuffer.data(), 2, false));

  CHECK(modBuffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[0], 0xFF)));
  CHECK(modBuffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[1], 0xFF)));
  CHECK(modBuffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[2], 0xFF)));
  CHECK(modBuffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(baseBuffer[3], 0xFF)));
}

TEST_CASE("AudioVoice updates live LFO values between internal render chunks") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0xFF, false);
  ConfigureLFO(instrument, 0, LFOTarget::VoiceAmp, LFOShape::Square, 223,
               LFOSettings::MaxRateValue);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));
  voice.lfoBank_.states_[0].phase_ = 0.49f;
  voice.lfoBank_.states_[0].currentValue_ = 1.0f;

  std::array<AudioSample, 256> buffer{};
  REQUIRE(voice.Render(buffer.data(), 128, false));

  AudioSample firstHalf = 0;
  AudioSample secondHalf = 0;
  for (int i = 0; i < 64; ++i) {
    firstHalf += std::abs(buffer[i * 2]) + std::abs(buffer[i * 2 + 1]);
  }
  for (int i = 64; i < 128; ++i) {
    secondHalf += std::abs(buffer[i * 2]) + std::abs(buffer[i * 2 + 1]);
  }

  CHECK(firstHalf > secondHalf);
}

TEST_CASE("AudioVoice advances separated stage LFOs on their own stages") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoicePan)->SetInt(0x7F, false);
  ConfigureLFO(instrument, 0, LFOTarget::VoiceBandpassCutoff, LFOShape::Square,
               0x20, LFOSettings::MaxRateValue);
  ConfigureLFO(instrument, 1, LFOTarget::VoicePan, LFOShape::Square, 0x7F,
               LFOSettings::MaxRateValue);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));
  voice.lfoBank_.states_[0].phase_ = 0.49f;
  voice.lfoBank_.states_[0].currentValue_ = 1.0f;
  voice.lfoBank_.states_[1].phase_ = 0.49f;
  voice.lfoBank_.states_[1].currentValue_ = 1.0f;

  std::array<AudioSample, 256> buffer{};
  REQUIRE(voice.Render(buffer.data(), 128, false));

  AudioSample firstRight = 0;
  AudioSample firstLeft = 0;
  AudioSample secondRight = 0;
  AudioSample secondLeft = 0;
  for (int i = 0; i < 64; ++i) {
    firstRight += std::abs(buffer[i * 2]);
    firstLeft += std::abs(buffer[i * 2 + 1]);
  }
  for (int i = 64; i < 128; ++i) {
    secondRight += std::abs(buffer[i * 2]);
    secondLeft += std::abs(buffer[i * 2 + 1]);
  }

  CHECK(firstRight > firstLeft);
  CHECK(secondLeft > secondRight);
}

TEST_CASE("AudioVoice applies live LFO values to ADSR sustain") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmp)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceAmpAttack)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpDecay)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpSustain)->SetInt(0x40, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0, false);
  ConfigureLFO(instrument, 0, LFOTarget::VoiceAmpSustain, LFOShape::Square,
               0x40, LFOSettings::MaxRateValue);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));
  voice.lfoBank_.states_[0].phase_ = 0.49f;
  voice.lfoBank_.states_[0].currentValue_ = 1.0f;

  std::array<AudioSample, 256> buffer{};
  REQUIRE(voice.Render(buffer.data(), 128, false));

  AudioSample firstHalf = 0;
  AudioSample secondHalf = 0;
  for (int i = 0; i < 64; ++i) {
    firstHalf += std::abs(buffer[i * 2]) + std::abs(buffer[i * 2 + 1]);
  }
  for (int i = 64; i < 128; ++i) {
    secondHalf += std::abs(buffer[i * 2]) + std::abs(buffer[i * 2 + 1]);
  }

  CHECK(firstHalf > secondHalf);
}
TEST_CASE("AudioVoice base-width stage keeps state across renders") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceBandpassCutoff)->SetInt(96, false);
  instrument.VoiceVar(FourCC::VoiceBandpassMix)->SetInt(32, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 8> first{};
  std::array<AudioSample, 8> second{};
  REQUIRE(voice.Render(first.data(), 4, false));
  REQUIRE(voice.Render(second.data(), 4, false));

  CHECK(sumAbs(second) != sumAbs(first));
  for (AudioSample value : first) {
    CHECK(std::abs(value) <= kAudioSampleMax);
  }
  for (AudioSample value : second) {
    CHECK(std::abs(value) <= kAudioSampleMax);
  }
}

TEST_CASE("AudioVoice standard filter lowpass output stays bounded") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(16000), pcmToAudioSample(16000));
  instrument.SetAlternating(true);
  instrument.VoiceVar(FourCC::VoiceFilterCutoff)->SetInt(64, false);
  instrument.VoiceVar(FourCC::VoiceFilterResonance)->SetInt(0xC0, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 8> buffer{};
  REQUIRE(voice.Render(buffer.data(), 4, false));

  bool differsFromSource = false;
  for (AudioSample value : buffer) {
    CHECK(std::abs(value) <= kAudioSampleMax);
    if (value != pcmToAudioSample(16000) && value != pcmToAudioSample(-16000)) {
      differsFromSource = true;
    }
  }
  CHECK(differsFromSource);
}

TEST_CASE(
    "AudioVoice filter type morphs from lowpass to bandpass to highpass") {
  const AudioSample lowpassDC = renderFilterTypeEnergy(0, false);
  const AudioSample bandpassDC = renderFilterTypeEnergy(0x80, false);
  const AudioSample highpassDC = renderFilterTypeEnergy(0xFF, false);

  CHECK(lowpassDC > bandpassDC * 8.0f);
  CHECK(lowpassDC > highpassDC * 8.0f);

  const AudioSample lowpassAlternating = renderFilterTypeEnergy(0, true);
  const AudioSample highpassAlternating = renderFilterTypeEnergy(0xFF, true);

  CHECK(highpassAlternating > lowpassAlternating * 8.0f);
}

TEST_CASE("AudioVoice release envelope fades out and stops the voice") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(1, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 16> warmup{};
  REQUIRE(voice.Render(warmup.data(), 8, false));

  voice.ReleaseInstrument();
  CHECK(instrument.ReleaseCount() == 1);
  voice.ReleaseInstrument();
  CHECK(instrument.ReleaseCount() == 1);

  bool sawReleaseOutput = false;
  bool stopped = false;
  std::array<AudioSample, 16> releaseBuffer{};
  for (int i = 0; i < 80; ++i) {
    if (!voice.Render(releaseBuffer.data(), 8, false)) {
      stopped = true;
      break;
    }
    sawReleaseOutput = sawReleaseOutput || sumAbs(releaseBuffer) > 0;
  }

  CHECK(sawReleaseOutput);
  CHECK(stopped);
}

TEST_CASE(
    "AudioVoice legato start preserves release timer and envelope state") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmpAttack)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpDecay)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpSustain)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0x80, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  // Set TLN to 2 (so it should release at tick 3)
  voice.ProcessCommand(TrackerCommand::InstrumentCommandGateLength, 2);

  std::array<AudioSample, 8> buffer{};
  // Tick 1
  REQUIRE(voice.Render(buffer.data(), 4, true));
  CHECK(instrument.ReleaseCount() == 0);

  // Tick 2 (Legato note starts here)
  REQUIRE(voice.StartInstrument(&instrument, 62, false));
  REQUIRE(voice.Render(buffer.data(), 4, true));
  CHECK(instrument.ReleaseCount() == 0);

  // Tick 3 (Should release now, because timer was NOT reset by legato)
  REQUIRE(voice.Render(buffer.data(), 4, true));
  CHECK(instrument.ReleaseCount() == 1);
}

TEST_CASE("AudioVoice trigger length zero does not schedule release") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceTriggerLength)->SetInt(0, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 8> buffer{};
  REQUIRE(voice.Render(buffer.data(), 4, true));
  REQUIRE(voice.Render(buffer.data(), 4, true));
  REQUIRE(voice.Render(buffer.data(), 4, true));

  CHECK(instrument.ReleaseCount() == 0);
}

TEST_CASE("AudioVoice trigger length releases after configured audio slices") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceTriggerLength)->SetInt(2, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0x80, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 8> buffer{};
  REQUIRE(voice.Render(buffer.data(), 4, true)); // Tick 1
  CHECK(instrument.ReleaseCount() == 0);

  REQUIRE(voice.Render(buffer.data(), 4, true)); // Tick 2
  CHECK(instrument.ReleaseCount() == 0);

  REQUIRE(voice.Render(buffer.data(), 4, true)); // Tick 3 -> release
  CHECK(instrument.ReleaseCount() == 1);

  voice.ReleaseInstrument();
  CHECK(instrument.ReleaseCount() == 1);
}

TEST_CASE("AudioVoice TLN command updates trigger length") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceTriggerLength)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0x80, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  voice.ProcessCommand(TrackerCommand::InstrumentCommandGateLength, 6);

  std::array<AudioSample, 8> buffer{};
  // Ticks 1 to 6
  for (int i = 1; i <= 6; ++i) {
    REQUIRE(voice.Render(buffer.data(), 4, true));
    CHECK(instrument.ReleaseCount() == 0);
  }

  // Tick 7
  REQUIRE(voice.Render(buffer.data(), 4, true));
  CHECK(instrument.ReleaseCount() == 1);
}

TEST_CASE("AudioVoice release follows live release parameter changes") {
  StubInstrument instrument;
  instrument.SetSignal(pcmToAudioSample(12000), pcmToAudioSample(12000));
  instrument.VoiceVar(FourCC::VoiceAmpAttack)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpDecay)->SetInt(0, false);
  instrument.VoiceVar(FourCC::VoiceAmpSustain)->SetInt(0xFF, false);
  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0x80, false);

  AudioVoice voice(0);
  REQUIRE(voice.StartInstrument(&instrument, 60, true));

  std::array<AudioSample, 16> warmup{};
  REQUIRE(voice.Render(warmup.data(), 8, false));
  CHECK(sumAbs(warmup) > 0);

  voice.ReleaseInstrument();

  std::array<AudioSample, 16> releasing{};
  REQUIRE(voice.Render(releasing.data(), 8, false));
  CHECK(sumAbs(releasing) > 0);

  instrument.VoiceVar(FourCC::VoiceAmpRelease)->SetInt(0, false);

  std::array<AudioSample, 16> killed{};
  REQUIRE(voice.Render(killed.data(), 8, false));
  CHECK(sumAbs(killed) == 0);
  CHECK_FALSE(voice.Render(killed.data(), 8, false));
}

TEST_CASE("AudioVoice filter envelope raises the standard filter cutoff") {
  StubInstrument baseInstrument;
  baseInstrument.SetSignal(pcmToAudioSample(16000), pcmToAudioSample(16000));
  baseInstrument.SetAlternating(true);
  baseInstrument.VoiceVar(FourCC::VoiceFilterCutoff)->SetInt(16, false);
  baseInstrument.VoiceVar(FourCC::VoiceFilterResonance)->SetInt(0, false);

  StubInstrument envInstrument;
  envInstrument.SetSignal(pcmToAudioSample(16000), pcmToAudioSample(16000));
  envInstrument.SetAlternating(true);
  envInstrument.VoiceVar(FourCC::VoiceFilterCutoff)->SetInt(16, false);
  envInstrument.VoiceVar(FourCC::VoiceFilterResonance)->SetInt(0, false);
  envInstrument.VoiceVar(FourCC::VoiceFilterEnvAmount)->SetInt(64, false);
  envInstrument.VoiceVar(FourCC::VoiceFilterAttack)->SetInt(0, false);
  envInstrument.VoiceVar(FourCC::VoiceFilterDecay)->SetInt(0, false);
  envInstrument.VoiceVar(FourCC::VoiceFilterSustain)->SetInt(0xFF, false);
  envInstrument.VoiceVar(FourCC::VoiceFilterRelease)->SetInt(0, false);

  AudioVoice baseVoice(0);
  AudioVoice envVoice(1);
  REQUIRE(baseVoice.StartInstrument(&baseInstrument, 60, true));
  REQUIRE(envVoice.StartInstrument(&envInstrument, 60, true));

  std::array<AudioSample, 16> baseBuffer{};
  std::array<AudioSample, 16> envBuffer{};
  REQUIRE(baseVoice.Render(baseBuffer.data(), 8, false));
  REQUIRE(envVoice.Render(envBuffer.data(), 8, false));

  CHECK(sumAbs(envBuffer) > sumAbs(baseBuffer));
}
