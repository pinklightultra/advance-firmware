/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "VoiceParameters.h"

#include "System/Console/n_assert.h"

#include <strings.h>

void VoiceParameters::InsertTypeVariable::SetString(const char *string,
                                                    bool notify) {
  NAssert(string);
  for (int32_t i = 0; i < kVoiceInsertEffectCount; ++i) {
    if (strcasecmp(string, kVoiceInsertEffects[i].name_) == 0 ||
        strcasecmp(string, kVoiceInsertEffects[i].displayName_) == 0) {
      Variable::SetInt(i, notify);
      return;
    }
  }

  Variable::SetInt(VAR_OFF, notify);
}

etl::string<MAX_VARIABLE_STRING_LENGTH>
VoiceParameters::InsertTypeVariable::GetString() {
  return GetVoiceInsertEffectInfo(GetInt()).displayName_;
}

VoiceParameters::VoiceParameters()
    : VariableContainer(&variables_),
      bandpassCutoff_(FourCC::VoiceBandpassCutoff, 0),
      bandpassResonance_(FourCC::VoiceBandpassResonance, 0),
      bandpassMix_(FourCC::VoiceBandpassMix, 0xFF),
      filterCutoff_(FourCC::VoiceFilterCutoff, 0xFF),
      filterResonance_(FourCC::VoiceFilterResonance, 0),
      filterType_(FourCC::VoiceFilterType, 0),
      filterMix_(FourCC::VoiceFilterMix, 0xFF),
      ampAttack_(FourCC::VoiceAmpAttack, 0),
      ampDecay_(FourCC::VoiceAmpDecay, 0),
      ampSustain_(FourCC::VoiceAmpSustain, 0xFF),
      ampRelease_(FourCC::VoiceAmpRelease, 0),
      filterAttack_(FourCC::VoiceFilterAttack, 0),
      filterDecay_(FourCC::VoiceFilterDecay, 0),
      filterSustain_(FourCC::VoiceFilterSustain, 0),
      filterRelease_(FourCC::VoiceFilterRelease, 0),
      filterEnvAmount_(FourCC::VoiceFilterEnvAmount, 0),
      amp_(FourCC::VoiceAmp, 0xFF), pan_(FourCC::VoicePan, 0x7F),
      triggerLength_(FourCC::VoiceTriggerLength, 0),
      insertType1_(GetVoiceInsertTypeFourCC(0), kDefaultVoiceInsertEffectIndex),
      insertAmount1_(GetVoiceInsertAmountFourCC(0), kDefaultVoiceInsertAmount),
      insertPosition1_(GetVoiceInsertPositionFourCC(0),
                       GetVoiceInsertPositionNames(), kVoiceInsertPositionCount,
                       kDefaultVoiceInsertPosition),
      insertType2_(GetVoiceInsertTypeFourCC(1), kDefaultVoiceInsertEffectIndex),
      insertAmount2_(GetVoiceInsertAmountFourCC(1), kDefaultVoiceInsertAmount),
      insertPosition2_(GetVoiceInsertPositionFourCC(1),
                       GetVoiceInsertPositionNames(), kVoiceInsertPositionCount,
                       kDefaultVoiceInsertPosition) {

  variables_.push_back(&bandpassCutoff_);
  variables_.push_back(&bandpassResonance_);
  variables_.push_back(&bandpassMix_);
  variables_.push_back(&filterCutoff_);
  variables_.push_back(&filterResonance_);
  variables_.push_back(&filterType_);
  variables_.push_back(&filterMix_);
  variables_.push_back(&ampAttack_);
  variables_.push_back(&ampDecay_);
  variables_.push_back(&ampSustain_);
  variables_.push_back(&ampRelease_);
  variables_.push_back(&filterAttack_);
  variables_.push_back(&filterDecay_);
  variables_.push_back(&filterSustain_);
  variables_.push_back(&filterRelease_);
  variables_.push_back(&filterEnvAmount_);
  variables_.push_back(&amp_);
  variables_.push_back(&pan_);
  variables_.push_back(&triggerLength_);
  insertType1_.SetAmountVariable(&insertAmount1_);
  variables_.push_back(&insertType1_);
  variables_.push_back(&insertAmount1_);
  variables_.push_back(&insertPosition1_);
  insertType2_.SetAmountVariable(&insertAmount2_);
  variables_.push_back(&insertType2_);
  variables_.push_back(&insertAmount2_);
  variables_.push_back(&insertPosition2_);
  lfoSettings_.AppendTo(variables_);
}

void VoiceParameters::AppendTo(etl::ivector<Variable *> &variables) {
  for (auto it = variables_.begin(); it != variables_.end(); ++it) {
    variables.insert(variables.end(), *it);
  }
}

void VoiceParameters::CopyFrom(const VoiceParameters &other) {
  auto src = other.variables_.begin();
  for (auto dst = variables_.begin();
       dst != variables_.end() && src != other.variables_.end(); ++dst, ++src) {
    (*dst)->CopyFrom(**src);
  }
}

void VoiceParameters::Reset() {
  for (auto it = variables_.begin(); it != variables_.end(); ++it) {
    (*it)->Reset();
  }
}
