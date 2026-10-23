/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DATTORRO_REVERB_OUTPUT_EFFECT_H_
#define _DATTORRO_REVERB_OUTPUT_EFFECT_H_

#include "DattorroReverb.h"
#include "OutputEffect.h"

class DattorroReverbOutputEffect final : public OutputEffect {
public:
  static constexpr size_t kInternalMemorySize =
      DattorroReverb::kInternalMemorySize;
  static constexpr size_t kExternalMemorySize =
      DattorroReverb::kExternalMemorySize;

  static constexpr int MinimumParameterValue =
      DattorroReverbSettings::MinimumParameterValue;
  static constexpr int MaximumParameterValue =
      DattorroReverbSettings::MaximumParameterValue;
  static constexpr int MaximumFiniteDecayParameterValue =
      DattorroReverbSettings::MaximumFiniteDecayValue;
  static constexpr int DefaultPreDelay =
      DattorroReverbSettings::DefaultPreDelay;
  static constexpr int DefaultInputTone =
      DattorroReverbSettings::DefaultInputTone;
  static constexpr int DefaultDiffuse = 0xB4;
  static constexpr int DefaultDecay = DattorroReverbSettings::DefaultDecay;
  static constexpr int DefaultTailTone =
      DattorroReverbSettings::DefaultTailTone;
  static constexpr int DefaultModulationRate =
      DattorroReverbSettings::DefaultModulationRate;
  static constexpr int DefaultModulationDepth =
      DattorroReverbSettings::DefaultModulationDepth;
  static constexpr int MinimumDiffusion =
      DattorroReverbSettings::MinimumDiffusion;
  // Diffusion coefficients stop just below unity to avoid the coloration and
  // buzzing that all-pass sections can produce as they approach 1.0.
  static constexpr int MaximumDiffusion =
      DattorroReverbSettings::MaximumDiffusion;
  static constexpr int DefaultPreDiffusion1 =
      DattorroReverbSettings::DefaultPreDiffusion1;
  static constexpr int DefaultPreDiffusion2 =
      DattorroReverbSettings::DefaultPreDiffusion2;
  static constexpr int DefaultDecayDiffusion =
      DattorroReverbSettings::DefaultDecayDiffusion;

  DattorroReverbOutputEffect();

  int32_t GetTailFrameCount() const override;
  void Render(const AudioSample *input, AudioSample *output,
              int sampleCount) override;
  void Reset() override;

private:
  static constexpr size_t VariableCount = 10;

  etl::ivector<Variable *> *Variables() override { return &variables_; }
  void ApplyParameters() override;
  void ApplyDiffuseMacro();
  void Update(Observable &observable, I_ObservableData *data) override;

  etl::vector<Variable *, VariableCount> variables_;
  WatchedVariable preDelay_;
  WatchedVariable inputTone_;
  WatchedVariable diffuse_;
  WatchedVariable preDiffusion1_;
  WatchedVariable preDiffusion2_;
  WatchedVariable decay_;
  WatchedVariable decayDiffusion_;
  WatchedVariable tailTone_;
  WatchedVariable modulationRate_;
  WatchedVariable modulationDepth_;
  DattorroReverb implementation_;
};

#endif
