/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _UI_FM6_PARAM_FIELD_H_
#define _UI_FM6_PARAM_FIELD_H_

#include "Application/Instruments/FM6Instrument.h"
#include "Foundation/Observable.h"
#include "UIField.h"

// Shared operator/global selection driven by UIFM6AlgoField's cursor:
// 1-6 select a DX7 operator, FM6SelectionGlobal selects the global block.
static constexpr uint8_t FM6SelectionGlobal = 7;

enum class FM6ParamFormat : uint8_t {
  Numeric, // raw value, zero padded to two digits when max >= 10
  Signed,  // raw value plus displayOffset_, always signed
  Names,   // names[value]
  Note,    // note name, value 0 is the DX7 break point A-1
};

// Describes one packed-record field: where its bits live and how to show
// them. Instances are static const, so a field only stores a pointer.
struct FM6ParamDef {
  uint8_t offset;    // byte offset within the slice
  uint8_t shift;     // bit position of the value within that byte
  uint8_t valueMask; // mask applied after shifting down
  uint8_t min;
  uint8_t max;
  uint8_t coarseStep; // LEFT/RIGHT step; UP/DOWN always step by one
  FM6ParamFormat format;
  int8_t displayOffset;
  const char *const *names; // Names format only
};

// Edits one parameter of the FM6 patch. The parameters live packed inside
// the instrument's seven hex-encoded slice variables rather than as
// individual Variables, so this field reads and writes the bits in place.
// Which slice it targets is resolved at draw/edit time from the shared
// selection that UIFM6AlgoField's cursor drives, so moving that cursor
// between operators re-points the whole panel without rebuilding it.
class UIFM6ParamField : public UIField, public Observable {
public:
  UIFM6ParamField(const GUIPoint &position, FM6Instrument &instrument,
                  const uint8_t &selection, const FM6ParamDef &def);
  virtual ~UIFM6ParamField(){};

  virtual void Draw(GUIWindow &w, int offset = 0);
  virtual void ProcessArrow(unsigned short mask);
  virtual void OnClick(){};

private:
  size_t sliceIndex() const;
  FourCC sliceID() const;
  uint8_t read() const;
  void write(uint8_t value);

  FM6Instrument &instrument_;
  const uint8_t &selection_;
  const FM6ParamDef &def_;
};

// Packed-record layout of every editable DX7 parameter. The operator
// entries are offsets within one 17-byte operator slice; the rest are
// offsets within the 16-byte global block.
extern const FM6ParamDef kFM6OpRate[4];
extern const FM6ParamDef kFM6OpLevel[4];
extern const FM6ParamDef kFM6OpBreakPoint;
extern const FM6ParamDef kFM6OpLeftDepth;
extern const FM6ParamDef kFM6OpRightDepth;
extern const FM6ParamDef kFM6OpLeftCurve;
extern const FM6ParamDef kFM6OpRightCurve;
extern const FM6ParamDef kFM6OpRateScaling;
extern const FM6ParamDef kFM6OpDetune;
extern const FM6ParamDef kFM6OpAmpModSens;
extern const FM6ParamDef kFM6OpVelSens;
extern const FM6ParamDef kFM6OpOutputLevel;
extern const FM6ParamDef kFM6OpFixedMode;
extern const FM6ParamDef kFM6OpCoarse;
extern const FM6ParamDef kFM6OpFine;

extern const FM6ParamDef kFM6PitchEgRate[4];
extern const FM6ParamDef kFM6PitchEgLevel[4];
extern const FM6ParamDef kFM6Feedback;
extern const FM6ParamDef kFM6OscSync;
extern const FM6ParamDef kFM6LfoSpeed;
extern const FM6ParamDef kFM6LfoDelay;
extern const FM6ParamDef kFM6LfoPitchDepth;
extern const FM6ParamDef kFM6LfoAmpDepth;
extern const FM6ParamDef kFM6LfoSync;
extern const FM6ParamDef kFM6LfoWave;
extern const FM6ParamDef kFM6PitchModSens;
extern const FM6ParamDef kFM6Transpose;

#endif
