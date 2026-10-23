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

#include "Project.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Instruments/SampleSourceInstrument.h"
#include "Application/Mixer/OutputEffectDefinitions.h"
#include "Application/Model/FirmwareVersion.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Player/SyncMaster.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "Groove.h"
#include "Scale.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/Trace.h"
#include "System/io/Status.h"
#include "Table.h"

#include <math.h>

#define DEFAULT_CHANNEL_VOLUME 99
#define DEFAULT_CHANNEL_FX_SEND 0
#define DEFAULT_FX_RETURN_VOLUME 99
#define DEFAULT_REVERB_INPUT_GAIN 20
#define DEFAULT_REVERB_TIME 35
#define DEFAULT_REVERB_DIFFUSION 62
#define DEFAULT_REVERB_LOWPASS 69
#define DEFAULT_PREVIEW_VOLUME 60

#define DEFAULT_MASTER_VOLUME 100

#define DATA_UNUSED_VALUE 0xFF

namespace {
constexpr const char *kProjectAudioModeNames[] = {"stereo", "mono"};
constexpr int kProjectAudioModeCount = 2;

bool IsMixRoutingVariable(FourCC id) {
  if (id == FourCC::VarMasterVolume) {
    return true;
  }
  if (id >= FourCC::VarChannel1Volume && id <= FourCC::VarChannel8Volume) {
    return true;
  }
  if (id >= FourCC::VarChannel1FX1Send && id <= FourCC::VarChannel8FX3Send) {
    return true;
  }
  if (id >= FourCC::VarFX1Volume && id <= FourCC::VarFX3Type) {
    return true;
  }
  return id >= FourCC::VarReverbInputGain && id <= FourCC::VarReverbLowPass;
}

} // namespace

Project::Project(const char *name)
    : Persistent("PROJECT"), VariableContainer(&variables_), song_(),
      tempoNudge_(0), tempo_(FourCC::VarTempo, DEFAULT_TEMPO),
      masterVolume_(FourCC::VarMasterVolume, DEFAULT_MASTER_VOLUME),
      channelVolume1_(FourCC::VarChannel1Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume2_(FourCC::VarChannel2Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume3_(FourCC::VarChannel3Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume4_(FourCC::VarChannel4Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume5_(FourCC::VarChannel5Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume6_(FourCC::VarChannel6Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume7_(FourCC::VarChannel7Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume8_(FourCC::VarChannel8Volume, DEFAULT_CHANNEL_VOLUME),
      channel1FX1Send_(FourCC::VarChannel1FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel2FX1Send_(FourCC::VarChannel2FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel3FX1Send_(FourCC::VarChannel3FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel4FX1Send_(FourCC::VarChannel4FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel5FX1Send_(FourCC::VarChannel5FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel6FX1Send_(FourCC::VarChannel6FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel7FX1Send_(FourCC::VarChannel7FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel8FX1Send_(FourCC::VarChannel8FX1Send, DEFAULT_CHANNEL_FX_SEND),
      channel1FX2Send_(FourCC::VarChannel1FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel2FX2Send_(FourCC::VarChannel2FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel3FX2Send_(FourCC::VarChannel3FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel4FX2Send_(FourCC::VarChannel4FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel5FX2Send_(FourCC::VarChannel5FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel6FX2Send_(FourCC::VarChannel6FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel7FX2Send_(FourCC::VarChannel7FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel8FX2Send_(FourCC::VarChannel8FX2Send, DEFAULT_CHANNEL_FX_SEND),
      channel1FX3Send_(FourCC::VarChannel1FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel2FX3Send_(FourCC::VarChannel2FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel3FX3Send_(FourCC::VarChannel3FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel4FX3Send_(FourCC::VarChannel4FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel5FX3Send_(FourCC::VarChannel5FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel6FX3Send_(FourCC::VarChannel6FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel7FX3Send_(FourCC::VarChannel7FX3Send, DEFAULT_CHANNEL_FX_SEND),
      channel8FX3Send_(FourCC::VarChannel8FX3Send, DEFAULT_CHANNEL_FX_SEND),
      fx1Volume_(FourCC::VarFX1Volume, DEFAULT_FX_RETURN_VOLUME),
      fx2Volume_(FourCC::VarFX2Volume, DEFAULT_FX_RETURN_VOLUME),
      fx3Volume_(FourCC::VarFX3Volume, DEFAULT_FX_RETURN_VOLUME),
      fx1Type_(FourCC::VarFX1Type, GetOutputEffectNames(),
               kOutputEffectTypeCount, kDefaultOutputEffectTypes[0]),
      fx2Type_(FourCC::VarFX2Type, GetOutputEffectNames(),
               kOutputEffectTypeCount, kDefaultOutputEffectTypes[1]),
      fx3Type_(FourCC::VarFX3Type, GetOutputEffectNames(),
               kOutputEffectTypeCount, kDefaultOutputEffectTypes[2]),
      reverbInputGain_(FourCC::VarReverbInputGain, DEFAULT_REVERB_INPUT_GAIN),
      reverbTime_(FourCC::VarReverbTime, DEFAULT_REVERB_TIME),
      reverbDiffusion_(FourCC::VarReverbDiffusion,
                       DEFAULT_REVERB_DIFFUSION),
      reverbLowPass_(FourCC::VarReverbLowPass, DEFAULT_REVERB_LOWPASS),
      wrap_(FourCC::VarWrap, false), transpose_(FourCC::VarTranspose, 0),
      scale_(FourCC::VarScale, scaleNames, numScales, 0),
      scaleRoot_(FourCC::VarScaleRoot, noteNames, 12, 0),
      projectName_(FourCC::VarProjectName, name),
      projectAudioMode_(FourCC::VarProjectAudioMode, kProjectAudioModeNames,
                        kProjectAudioModeCount, kProjectAudioModeStereo),
      previewVolume_(FourCC::VarPreviewVolume, DEFAULT_PREVIEW_VOLUME),
      mixRoutingDirty_(true) {

  this->variables_.insert(variables_.end(), &tempo_);
  this->variables_.insert(variables_.end(), &masterVolume_);

  // Add individual channel volume variables to the container
  this->variables_.insert(variables_.end(), &channelVolume1_);
  this->variables_.insert(variables_.end(), &channelVolume2_);
  this->variables_.insert(variables_.end(), &channelVolume3_);
  this->variables_.insert(variables_.end(), &channelVolume4_);
  this->variables_.insert(variables_.end(), &channelVolume5_);
  this->variables_.insert(variables_.end(), &channelVolume6_);
  this->variables_.insert(variables_.end(), &channelVolume7_);
  this->variables_.insert(variables_.end(), &channelVolume8_);
  this->variables_.insert(variables_.end(), &channel1FX1Send_);
  this->variables_.insert(variables_.end(), &channel2FX1Send_);
  this->variables_.insert(variables_.end(), &channel3FX1Send_);
  this->variables_.insert(variables_.end(), &channel4FX1Send_);
  this->variables_.insert(variables_.end(), &channel5FX1Send_);
  this->variables_.insert(variables_.end(), &channel6FX1Send_);
  this->variables_.insert(variables_.end(), &channel7FX1Send_);
  this->variables_.insert(variables_.end(), &channel8FX1Send_);
  this->variables_.insert(variables_.end(), &channel1FX2Send_);
  this->variables_.insert(variables_.end(), &channel2FX2Send_);
  this->variables_.insert(variables_.end(), &channel3FX2Send_);
  this->variables_.insert(variables_.end(), &channel4FX2Send_);
  this->variables_.insert(variables_.end(), &channel5FX2Send_);
  this->variables_.insert(variables_.end(), &channel6FX2Send_);
  this->variables_.insert(variables_.end(), &channel7FX2Send_);
  this->variables_.insert(variables_.end(), &channel8FX2Send_);
  this->variables_.insert(variables_.end(), &channel1FX3Send_);
  this->variables_.insert(variables_.end(), &channel2FX3Send_);
  this->variables_.insert(variables_.end(), &channel3FX3Send_);
  this->variables_.insert(variables_.end(), &channel4FX3Send_);
  this->variables_.insert(variables_.end(), &channel5FX3Send_);
  this->variables_.insert(variables_.end(), &channel6FX3Send_);
  this->variables_.insert(variables_.end(), &channel7FX3Send_);
  this->variables_.insert(variables_.end(), &channel8FX3Send_);
  this->variables_.insert(variables_.end(), &fx1Volume_);
  this->variables_.insert(variables_.end(), &fx2Volume_);
  this->variables_.insert(variables_.end(), &fx3Volume_);
  this->variables_.insert(variables_.end(), &fx1Type_);
  this->variables_.insert(variables_.end(), &fx2Type_);
  this->variables_.insert(variables_.end(), &fx3Type_);
  this->variables_.insert(variables_.end(), &reverbInputGain_);
  this->variables_.insert(variables_.end(), &reverbTime_);
  this->variables_.insert(variables_.end(), &reverbDiffusion_);
  this->variables_.insert(variables_.end(), &reverbLowPass_);

  this->variables_.insert(variables_.end(), &wrap_);
  this->variables_.insert(variables_.end(), &transpose_);
  this->variables_.insert(variables_.end(), &scale_);
  scale_.SetInt(0);
  this->variables_.insert(variables_.end(), &scaleRoot_);
  scaleRoot_.SetInt(0); // Default to C (0)
  this->variables_.insert(variables_.end(), &projectName_);
  this->variables_.insert(variables_.end(), &projectAudioMode_);
  this->variables_.insert(variables_.end(), &previewVolume_);

  WatchedVariable *mixRoutingVars[] = {
      &masterVolume_,
      &channelVolume1_,
      &channelVolume2_,
      &channelVolume3_,
      &channelVolume4_,
      &channelVolume5_,
      &channelVolume6_,
      &channelVolume7_,
      &channelVolume8_,
      &channel1FX1Send_,
      &channel2FX1Send_,
      &channel3FX1Send_,
      &channel4FX1Send_,
      &channel5FX1Send_,
      &channel6FX1Send_,
      &channel7FX1Send_,
      &channel8FX1Send_,
      &channel1FX2Send_,
      &channel2FX2Send_,
      &channel3FX2Send_,
      &channel4FX2Send_,
      &channel5FX2Send_,
      &channel6FX2Send_,
      &channel7FX2Send_,
      &channel8FX2Send_,
      &channel1FX3Send_,
      &channel2FX3Send_,
      &channel3FX3Send_,
      &channel4FX3Send_,
      &channel5FX3Send_,
      &channel6FX3Send_,
      &channel7FX3Send_,
      &channel8FX3Send_,
      &fx1Volume_,
      &fx2Volume_,
      &fx3Volume_,
      &fx1Type_,
      &fx2Type_,
      &fx3Type_,
      &reverbInputGain_,
      &reverbTime_,
      &reverbDiffusion_,
      &reverbLowPass_,
  };
  for (WatchedVariable *var : mixRoutingVars) {
    var->AddObserver(*this);
  }

  // Project name is now managed through the WatchedVariable

  // look if we can find a sav file

  // Makes sure the tables exists for restoring

  TableHolder::GetInstance();

  Groove::GetInstance()->Clear();

  tempoTapCount_ = 0;
  SyncSongChannelCount();

  Status::Set("About to load project");
};

Project::~Project() {}

void Project::Load(const char *name) {
  tempoNudge_ = 0;
  tempoTapCount_ = 0;
  for (int i = 0; i < MAX_TAP; i++) {
    lastTap_[i] = 0;
  }

  song_.Reset();
  instrumentBank_.Reset();
  Groove::GetInstance()->Clear();

  tempo_.Reset();
  masterVolume_.Reset();
  channelVolume1_.Reset();
  channelVolume2_.Reset();
  channelVolume3_.Reset();
  channelVolume4_.Reset();
  channelVolume5_.Reset();
  channelVolume6_.Reset();
  channelVolume7_.Reset();
  channelVolume8_.Reset();
  channel1FX1Send_.Reset();
  channel2FX1Send_.Reset();
  channel3FX1Send_.Reset();
  channel4FX1Send_.Reset();
  channel5FX1Send_.Reset();
  channel6FX1Send_.Reset();
  channel7FX1Send_.Reset();
  channel8FX1Send_.Reset();
  channel1FX2Send_.Reset();
  channel2FX2Send_.Reset();
  channel3FX2Send_.Reset();
  channel4FX2Send_.Reset();
  channel5FX2Send_.Reset();
  channel6FX2Send_.Reset();
  channel7FX2Send_.Reset();
  channel8FX2Send_.Reset();
  channel1FX3Send_.Reset();
  channel2FX3Send_.Reset();
  channel3FX3Send_.Reset();
  channel4FX3Send_.Reset();
  channel5FX3Send_.Reset();
  channel6FX3Send_.Reset();
  channel7FX3Send_.Reset();
  channel8FX3Send_.Reset();
  fx1Volume_.Reset();
  fx2Volume_.Reset();
  fx3Volume_.Reset();
  fx1Type_.Reset();
  fx2Type_.Reset();
  fx3Type_.Reset();
  reverbInputGain_.Reset();
  reverbTime_.Reset();
  reverbDiffusion_.Reset();
  reverbLowPass_.Reset();
  wrap_.Reset();
  transpose_.Reset();
  scale_.Reset();
  scaleRoot_.Reset();
  projectAudioMode_.Reset();
  previewVolume_.Reset();

  if (name) {
    projectName_.SetString(name, true);
  } else {
    projectName_.SetString("", true);
  }

  mixRoutingDirty_ = true;
  SyncSongChannelCount();

  Status::Set("About to load project");
}

int Project::GetScale() {
  Variable *v = FindVariable(FourCC::VarScale);
  NAssert(v);
  return v->GetInt();
}

uint8_t Project::GetScaleRoot() {
  Variable *v = FindVariable(FourCC::VarScaleRoot);
  NAssert(v);
  return v->GetInt();
}

int Project::GetTempo() {
  Variable *v = FindVariable(FourCC::VarTempo);
  NAssert(v);
  int tempo = v->GetInt() + tempoNudge_;
  return tempo;
};

int Project::GetAudioMode() {
  Variable *v = FindVariable(FourCC::VarProjectAudioMode);
  NAssert(v);
  return v->GetInt();
}

int Project::GetMasterVolume() {
  Variable *v = FindVariable(FourCC::VarMasterVolume);
  NAssert(v);
  return v->GetInt();
}

int Project::GetChannelVolume(int channel) {
  if (channel >= SONG_CHANNEL_COUNT_STEREO && channel < SONG_CHANNEL_COUNT) {
    return DEFAULT_CHANNEL_VOLUME;
  }

  switch (channel) {
  case 0:
    return channelVolume1_.GetInt();
  case 1:
    return channelVolume2_.GetInt();
  case 2:
    return channelVolume3_.GetInt();
  case 3:
    return channelVolume4_.GetInt();
  case 4:
    return channelVolume5_.GetInt();
  case 5:
    return channelVolume6_.GetInt();
  case 6:
    return channelVolume7_.GetInt();
  case 7:
    return channelVolume8_.GetInt();
  default:
    NAssert(false);
    return 0;
  }
};

int Project::GetChannelFXSend(int channel, int fx) {
  if (channel >= SONG_CHANNEL_COUNT_STEREO && channel < SONG_CHANNEL_COUNT) {
    return DEFAULT_CHANNEL_FX_SEND;
  }

  switch (fx) {
  case 0:
    switch (channel) {
    case 0:
      return channel1FX1Send_.GetInt();
    case 1:
      return channel2FX1Send_.GetInt();
    case 2:
      return channel3FX1Send_.GetInt();
    case 3:
      return channel4FX1Send_.GetInt();
    case 4:
      return channel5FX1Send_.GetInt();
    case 5:
      return channel6FX1Send_.GetInt();
    case 6:
      return channel7FX1Send_.GetInt();
    case 7:
      return channel8FX1Send_.GetInt();
    default:
      NAssert(false);
      return 0;
    }
  case 1:
    switch (channel) {
    case 0:
      return channel1FX2Send_.GetInt();
    case 1:
      return channel2FX2Send_.GetInt();
    case 2:
      return channel3FX2Send_.GetInt();
    case 3:
      return channel4FX2Send_.GetInt();
    case 4:
      return channel5FX2Send_.GetInt();
    case 5:
      return channel6FX2Send_.GetInt();
    case 6:
      return channel7FX2Send_.GetInt();
    case 7:
      return channel8FX2Send_.GetInt();
    default:
      NAssert(false);
      return 0;
    }
  case 2:
    switch (channel) {
    case 0:
      return channel1FX3Send_.GetInt();
    case 1:
      return channel2FX3Send_.GetInt();
    case 2:
      return channel3FX3Send_.GetInt();
    case 3:
      return channel4FX3Send_.GetInt();
    case 4:
      return channel5FX3Send_.GetInt();
    case 5:
      return channel6FX3Send_.GetInt();
    case 6:
      return channel7FX3Send_.GetInt();
    case 7:
      return channel8FX3Send_.GetInt();
    default:
      NAssert(false);
      return 0;
    }
  default:
    NAssert(false);
    return 0;
  }
}

int Project::GetFXReturnVolume(int fx) {
  switch (fx) {
  case 0:
    return fx1Volume_.GetInt();
  case 1:
    return fx2Volume_.GetInt();
  case 2:
    return fx3Volume_.GetInt();
  default:
    NAssert(false);
    return 0;
  }
}

int Project::GetFXSlotType(int fx) {
  switch (fx) {
  case 0:
    return fx1Type_.GetInt();
  case 1:
    return fx2Type_.GetInt();
  case 2:
    return fx3Type_.GetInt();
  default:
    NAssert(false);
    return kDefaultOutputEffectTypeIndex;
  }
}

int Project::GetReverbInputGain() { return reverbInputGain_.GetInt(); }

int Project::GetReverbTime() { return reverbTime_.GetInt(); }

int Project::GetReverbDiffusion() { return reverbDiffusion_.GetInt(); }

int Project::GetReverbLowPass() { return reverbLowPass_.GetInt(); }

bool Project::IsMixRoutingDirty() const { return mixRoutingDirty_; }

void Project::ClearMixRoutingDirty() { mixRoutingDirty_ = false; }

void Project::GetProjectName(char *name) {
  Variable *v = FindVariable(FourCC::VarProjectName);
  strncpy(name, v->GetString().c_str(), MAX_PROJECT_NAME_LENGTH);
  name[MAX_PROJECT_NAME_LENGTH] = '\0';
}

void Project::SetProjectName(char *name) {
  Variable *v = FindVariable(FourCC::VarProjectName);
  char buffer[MAX_PROJECT_NAME_LENGTH + 1];
  strncpy(buffer, name, MAX_PROJECT_NAME_LENGTH);
  buffer[MAX_PROJECT_NAME_LENGTH] = '\0';
  v->SetString(buffer, true);
}

void Project::NudgeTempo(int value) {
  if ((GetTempo() + tempoNudge_) > 0) {
    tempoNudge_ += value;
  }
};

void Project::Trigger() {
  if (tempoNudge_ != 0) {
    if (tempoNudge_ > 0) {
      tempoNudge_--;
    } else {
      tempoNudge_++;
    };
  }
};

int Project::GetTranspose() {
  Variable *v = FindVariable(FourCC::VarTranspose);
  NAssert(v);
  int result = v->GetInt();
  if (result > 0x80) {
    result -= 128;
  }
  return result;
};

bool Project::Wrap() {
  Variable *v = FindVariable(FourCC::VarWrap);
  NAssert(v);
  return v->GetBool();
};

InstrumentBank *Project::GetInstrumentBank() { return &instrumentBank_; };

void Project::Update(Observable &o, I_ObservableData *d) {
  if (!d) {
    return;
  }

  FourCC id =
      FourCC(static_cast<FourCC::value_type>(reinterpret_cast<uintptr_t>(d)));
  if (IsMixRoutingVariable(id)) {
    mixRoutingDirty_ = true;
  }
}

void Project::Purge() {

  song_.chain_.ClearAllocation();
  song_.phrase_.ClearAllocation();

  unsigned char *data = song_.data_;
  const int channelCount = song_.GetChannelCount();
  for (int i = 0; i < 256 * channelCount; i++) {
    if (*data != DATA_UNUSED_VALUE) {
      song_.chain_.SetUsed(*data);
    }
    data++;
  }

  data = song_.chain_.data_;
  unsigned char *data2 = song_.chain_.transpose_;

  for (int i = 0; i < CHAIN_COUNT; i++) {

    if (song_.chain_.IsUsed(i)) {
      for (int j = 0; j < 16; j++) {
        if (*data != DATA_UNUSED_VALUE) {
          song_.phrase_.SetUsed(*data);
        }
        data++;
        data2++;
      }
    } else {

      for (int j = 0; j < 16; j++) {
        *data++ = DATA_UNUSED_VALUE;
        *data2++ = 0x00;
      }
    }
  }

  data = song_.phrase_.note_;
  data2 = song_.phrase_.instr_;

  TrackerCommand *cmd1 = song_.phrase_.cmd1_;
  ushort *param1 = song_.phrase_.param1_;
  TrackerCommand *cmd2 = song_.phrase_.cmd2_;
  ushort *param2 = song_.phrase_.param2_;

  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < 16; j++) {
      if (!song_.phrase_.IsUsed(i)) {
        *data = DATA_UNUSED_VALUE;
        *data2 = DATA_UNUSED_VALUE;
        *cmd1 = TrackerCommand::InstrumentCommandNone;
        *param1 = 0;
        *cmd2 = TrackerCommand::InstrumentCommandNone;
        *param2 = 0;
      }
      data++;
      data2++;
      cmd1++;
      param1++;
      cmd2++;
      param2++;
    };
  }
};

// Returns the first instrument index using the sample, or -1 if unused.
int32_t Project::SampleInUse(
    etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename) {
  InstrumentBank *bank = GetInstrumentBank();

  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    I_Instrument *instrument = bank->GetInstrument(i);
    if (instrument->GetType() == IT_SAMPLE) {
      SampleInstrument *si = static_cast<SampleInstrument *>(instrument);
      if (si->GetSampleFileName() == filename) {
        return i;
      }
    } else if (instrument->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *ssi =
          static_cast<SampleSourceInstrument *>(instrument);
      if (ssi->GetSampleFileName() == filename) {
        return i;
      }
    }
  }
  return -1;
}

// remove all samples that are not in use by an instrument
void Project::PurgeSamples() {
  // clear used flag
  bool isUsed[MAX_SAMPLES] = {false};

  // flag all samples actually used
  InstrumentBank *bank = GetInstrumentBank();
  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    I_Instrument *instrument = bank->GetInstrument(i);
    if (instrument->GetType() == IT_SAMPLE) {
      SampleInstrument *si = static_cast<SampleInstrument *>(instrument);
      int index = si->GetSampleIndex();
      if (index >= 0) {
        isUsed[index] = true;
      }
    } else if (instrument->GetType() == IT_SAMPLE_SOURCE) {
      SampleSourceInstrument *ssi =
          static_cast<SampleSourceInstrument *>(instrument);
      int index = ssi->GetSampleIndex();
      if (index >= 0) {
        isUsed[index] = true;
      }
    }
  }

  // Now remove all unused samples from disk
  int purged = 0;
  SamplePool *sp = SamplePool::GetInstance();
  for (int i = 0; i < MAX_SAMPLES; i++) {
    if ((!isUsed[i]) && (sp->GetSource(i - purged))) {
      sp->PurgeSample(i - purged, projectName_.GetString().c_str());
      Trace::Debug("Purged sample [%d]", i - purged);
      purged++;
    } else {
      Trace::Debug("Sample [%d] not purged", i);
    }
  };
  Trace::Debug("Purged %d samples", purged);
}

void Project::PurgeInstruments() {

  bool used[MAX_INSTRUMENT_COUNT] = {false};
  unsigned char *data = song_.phrase_.instr_;

  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < 16; j++) {
      if (*data != DATA_UNUSED_VALUE) {
        NAssert(*data < MAX_INSTRUMENT_COUNT);
        used[*data] = true;
      }
      data++;
    }
  }

  InstrumentBank *bank = GetInstrumentBank();
  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    if (!used[i]) {
      I_Instrument *instrument = bank->GetInstrument(i);
      if (instrument->GetType() == IT_SAMPLE) {
        SampleInstrument *sampleInstrument =
            static_cast<SampleInstrument *>(instrument);
        sampleInstrument->AssignSample(NO_SAMPLE);
      } else if (instrument->GetType() == IT_SAMPLE_SOURCE) {
        SampleSourceInstrument *sampleSourceInstrument =
            static_cast<SampleSourceInstrument *>(instrument);
        sampleSourceInstrument->AssignSample(-1);
      }
      // we dont reorder indexes on release so safe to call inside this loop
      bank->releaseInstrument(i);
      Trace::Debug("Set unused instrument slot [%d] to NONE", i);
    }
  }
};

void Project::RestoreContent(PersistencyDocument *doc) {
  bool attr = doc->NextAttribute();
  while (attr) {
    attr = doc->NextAttribute();
  }

  // Now loop on all variables
  bool elem = doc->FirstChild();
  while (elem) {
    bool attr = doc->NextAttribute();
    char name[MAX_VARIABLE_STRING_LENGTH + 1];
    char value[MAX_VARIABLE_STRING_LENGTH + 1];
    while (attr) {
      if (!strcmp(doc->attrname_, "NAME")) {
        strcpy(name, doc->attrval_);
      }
      if (!strcmp(doc->attrname_, "VALUE")) {
        strcpy(value, doc->attrval_);
      }
      attr = doc->NextAttribute();
    }
    Variable *v = FindVariable(name);
    // Project name now comes from the directory, so ignore any persisted value.
    if (v && v->GetID() != FourCC::VarProjectName) {
      if (v->GetID() == FourCC::VarTempo) {
        v->SetInt(std::clamp(atoi(value), int(MIN_TEMPO), int(MAX_TEMPO)));
      } else {
        v->SetString(value);
      }
    }
    elem = doc->NextSibling();
  }
  SyncSongChannelCount();
  mixRoutingDirty_ = true;
}

void Project::SaveContent(tinyxml2::XMLPrinter *printer) {

  // store project version
  const FirmwareNumberString firmwareVersion = FormatFirmwareVersionNumber();
  printer->PushAttribute("VERSION", firmwareVersion.c_str());

  // save all of the project's parameters
  auto it = variables_.begin();
  for (size_t i = 0; i < variables_.size(); i++) {
    Variable *currentVar = *it;
    // Persist everything except the project name, which is derived from the
    // projects directory name
    if (currentVar->GetID() == FourCC::VarProjectName) {
      it++;
      continue;
    }

    printer->OpenElement("PARAMETER");
    printer->PushAttribute("NAME", currentVar->GetName());
    printer->PushAttribute("VALUE", currentVar->GetString().c_str());
    printer->CloseElement();
    it++;
  }
};

void Project::OnTempoTap() {

  unsigned long now = System::GetInstance()->GetClock();

  if (tempoTapCount_ != 0) {
    // count last tick tempo and see if in range
    unsigned millisec = now - lastTap_[tempoTapCount_ - 1];
    int t = int(60000 / (float)millisec);
    if (t > 30) {
      if (tempoTapCount_ == MAX_TAP) {
        for (int i = 0; i < int(tempoTapCount_) - 1; i++) {
          lastTap_[i] = lastTap_[i + 1];
        }
      } else {
        tempoTapCount_++;
      }
      int tempo =
          int(60000 * (tempoTapCount_ - 1) / (float)(now - lastTap_[0]));
      Variable *v = FindVariable(FourCC::VarTempo);
      // ensure tempo is within range
      tempo = std::clamp((unsigned short)tempo, MIN_TEMPO, MAX_TEMPO);
      v->SetInt(tempo);
    } else {
      tempoTapCount_ = 1;
    }
  } else {
    tempoTapCount_ = 1;
  }
  lastTap_[tempoTapCount_ - 1] = now;
}

void Project::SyncSongChannelCount() {
  const int channelCount = projectAudioMode_.GetInt() == kProjectAudioModeMono
                               ? SONG_CHANNEL_COUNT_MONO
                               : SONG_CHANNEL_COUNT_STEREO;
  song_.SetChannelCount(channelCount);
}
