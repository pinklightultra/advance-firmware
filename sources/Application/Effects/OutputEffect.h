/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_H_
#define _OUTPUT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "Externals/etl/include/etl/vector.h"
#include "Foundation/Observable.h"
#include "Foundation/Variables/VariableContainer.h"
#include "Foundation/Variables/WatchedVariable.h"

#include <cstdint>

class OutputEffect : public VariableContainer, public I_Observer {
public:
  explicit OutputEffect(etl::ivector<Variable *> *variables);
  virtual ~OutputEffect() {}

  virtual void Reset() {}
  void ResetParameters();
  virtual int32_t GetTailFrameCount() const { return 0; }
  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) = 0;

  // Parameter values and processing state are owned by the same effect.
  // Applying values immediately before rendering keeps the mixer independent
  // from concrete effect parameters while avoiding work until a value changes.
  void PrepareRender();
  etl::ivector<Variable *> &GetVariables() { return *Variables(); }

  void Update(Observable &observable, I_ObservableData *data) override;

protected:
  virtual etl::ivector<Variable *> *Variables() = 0;
  virtual void ApplyParameters() {}

  // Effect parameters notify their owning effect so processing values are
  // refreshed immediately before the next active render.
  void ObserveParameter(WatchedVariable &parameter);

private:
  bool parametersDirty_ = true;
};

// NONE occupies fixed positions reserved for effects that have not yet been
// implemented. It deliberately produces no wet output.
class NoneOutputEffect final : public OutputEffect {
public:
  NoneOutputEffect();
  void Render(const AudioSample *input, AudioSample *output,
              int samplecount) override;

private:
  etl::ivector<Variable *> *Variables() override { return &variables_; }

  etl::vector<Variable *, 1> variables_;
};

#endif
