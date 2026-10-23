/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "OutputEffect.h"

#include <cstring>

OutputEffect::OutputEffect(etl::ivector<Variable *> *variables)
    : VariableContainer(variables) {}

void OutputEffect::ObserveParameter(WatchedVariable &parameter) {
  parameter.AddObserver(*this);
}

void OutputEffect::Update(Observable &, I_ObservableData *) {
  parametersDirty_ = true;
}

void OutputEffect::PrepareRender() {
  if (parametersDirty_) {
    ApplyParameters();
    parametersDirty_ = false;
  }
}

void OutputEffect::ResetParameters() {
  for (Variable *variable : *Variables()) {
    variable->Reset();
  }
  parametersDirty_ = true;
}

NoneOutputEffect::NoneOutputEffect() : OutputEffect(&variables_) {}

void NoneOutputEffect::Render(const AudioSample *input, AudioSample *output,
                              int samplecount) {
  (void)input;
  memset(output, 0,
         static_cast<size_t>(samplecount) * 2U * sizeof(AudioSample));
}
