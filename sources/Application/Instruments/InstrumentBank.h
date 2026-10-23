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

#ifndef _INSTRUMENT_BANK_H_
#define _INSTRUMENT_BANK_H_

#include "Application/Instruments/I_Instrument.h"
#include "Application/Model/Song.h"
#include "Application/Persistency/Persistent.h"
#include "Externals/etl/include/etl/variant_pool.h"
#include "MidiInstrument.h"
#include "NoneInstrument.h"
#include "SampleInstrument.h"
#include "SampleSourceInstrument.h"
#include "VoiceParameters.h"

#define NO_MORE_INSTRUMENT 0x100

class InstrumentBank : public Persistent {
public:
  InstrumentBank();
  ~InstrumentBank();
  void Reset();
  void AssignDefaults();
  I_Instrument *GetInstrument(int i);
  virtual void SaveContent(tinyxml2::XMLPrinter *printer);
  virtual void RestoreContent(PersistencyDocument *doc);
  void Init();
  void OnStart();
  unsigned short GetNextAndAssignID(InstrumentType type, unsigned char id);
  void releaseInstrument(unsigned short id);
  unsigned short Clone(unsigned short i);
  unsigned short GetNextFreeInstrumentSlotId();
  VoiceParameters *GetVoiceParameters(int i);

  const etl::array<I_Instrument *, MAX_INSTRUMENT_COUNT> &
  InstrumentsList() const {
    return instruments_;
  }

private:
  void attachVoiceParameters(int id, I_Instrument *instrument);

  etl::array<I_Instrument *, MAX_INSTRUMENT_COUNT> instruments_;
  etl::array<VoiceParameters, MAX_INSTRUMENT_COUNT> voiceParameters_;
  etl::variant_pool<MAX_INSTRUMENT_COUNT, SampleInstrument,
                    SampleSourceInstrument, MidiInstrument>
      instrumentPool_;
  NoneInstrument none_ = NoneInstrument();
  unsigned short sidOscCount = 0;
};

#endif
