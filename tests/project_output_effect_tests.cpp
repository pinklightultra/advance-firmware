/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Effects/DattorroReverbOutputEffect.h"
#include "Application/Effects/DelayOutputEffect.h"
#include "Application/Model/Project.h"

TEST_CASE("Project owns four fixed output effect processors") {
  Project project("fx-owned-parameters");
  OutputEffect *fx1 = project.GetOutputEffect(kOutputEffectSlot1Index);
  OutputEffect *fx2 = project.GetOutputEffect(kOutputEffectSlot2Index);
  OutputEffect *delay = project.GetOutputEffect(kOutputEffectDelayIndex);
  OutputEffect *reverb = project.GetOutputEffect(kOutputEffectReverbIndex);
  REQUIRE(fx1 != nullptr);
  REQUIRE(fx2 != nullptr);
  REQUIRE(delay != nullptr);
  REQUIRE(reverb != nullptr);

  Variable *delayTime = delay->FindVariable(FourCC::VarDelayTime);
  Variable *reverbPreDelay = reverb->FindVariable(FourCC::VarReverbPreDelay);
  REQUIRE(delayTime != nullptr);
  REQUIRE(reverbPreDelay != nullptr);
  CHECK(fx1->FindVariable(FourCC::VarDelayTime) == nullptr);
  CHECK(fx2->FindVariable(FourCC::VarReverbPreDelay) == nullptr);
  CHECK(delay->FindVariable(FourCC::VarReverbPreDelay) == nullptr);
  CHECK(reverb->FindVariable(FourCC::VarDelayTime) == nullptr);

  delayTime->SetInt(18);
  reverbPreDelay->SetInt(84);

  CHECK(delayTime->GetInt() == 18);
  CHECK(reverbPreDelay->GetInt() == 84);
  CHECK(project.FindVariable(FourCC::VarDelayTime) == delayTime);
  CHECK(project.FindVariable(FourCC::VarReverbPreDelay) == reverbPreDelay);
  CHECK(project.GetOutputEffect(-1) == nullptr);
  CHECK(project.GetOutputEffect(OUTPUT_EFFECT_COUNT) == nullptr);
}

TEST_CASE("Loading a project resets fixed output effect parameters") {
  Project project("fx-reset-parameters");
  Variable *delayTime = project.FindVariable(FourCC::VarDelayTime);
  Variable *reverbPreDelay = project.FindVariable(FourCC::VarReverbPreDelay);
  REQUIRE(delayTime != nullptr);
  REQUIRE(reverbPreDelay != nullptr);
  delayTime->SetInt(18);
  reverbPreDelay->SetInt(84);

  project.Load("fx-reset-parameters");

  CHECK(delayTime->GetInt() == DelayOutputEffect::DefaultTime);
  CHECK(reverbPreDelay->GetInt() ==
        DattorroReverbOutputEffect::DefaultPreDelay);
}

TEST_CASE("Project defaults forward output effect routing to silence") {
  Project project("fx-routing-defaults");

  CHECK(project.GetFXSend(0, 1) == 0);
  CHECK(project.GetFXSend(0, 2) == 0);
  CHECK(project.GetFXSend(0, 3) == 0);
  CHECK(project.GetFXSend(1, 2) == 0);
  CHECK(project.GetFXSend(1, 3) == 0);
  CHECK(project.GetFXSend(2, 3) == 0);
}

TEST_CASE("Project defaults effect returns to full eight-bit level") {
  Project project("fx-return-defaults");

  for (int32_t effect = 0; effect < OUTPUT_EFFECT_COUNT; ++effect) {
    CHECK(project.GetFXReturnVolume(effect) == OutputEffectReturnMaximum);
  }
}

TEST_CASE("Output effect sends preserve the full eight-bit range") {
  Project project("fx-send-routing");
  Variable *channelSend = project.FindVariable(FourCC::VarChannel1FX3Send);
  Variable *effectSend = project.FindVariable(FourCC::VarFX1ToFX3Send);
  REQUIRE(channelSend != nullptr);
  REQUIRE(effectSend != nullptr);
  project.ClearMixRoutingDirty();

  channelSend->SetInt(OutputEffectSendMaximum, true);
  effectSend->SetInt(OutputEffectSendMaximum, true);

  CHECK(project.GetChannelFXSend(0, kOutputEffectDelayIndex) ==
        OutputEffectSendMaximum);
  CHECK(project.GetFXSend(kOutputEffectSlot1Index, kOutputEffectDelayIndex) ==
        OutputEffectSendMaximum);
  CHECK(project.IsMixRoutingDirty());
}
