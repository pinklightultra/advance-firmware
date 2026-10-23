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

#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

#include "Externals/etl/include/etl/enum_type.h"
#include <stdint.h>

struct TrackerCommand {
  // While the names of the command codes can be changed, their values CANNOT.
  // Values are used as is in save files, so any changes would cause save files
  // to break.
  enum enum_type : uint8_t {
    InstrumentCommandArpeggiator = 0,          // ARPG
    InstrumentCommandChance = 188,             // CHN
    InstrumentCommandCrush = 2,                // CRSH
    InstrumentCommandDelay = 4,                // DLAY
    InstrumentCommandFilterCut = 20,           // FCUT
    InstrumentCommandLowPassFilter = 22,       // FLTR
    InstrumentCommandFilterResonance = 25,     // FRES
    InstrumentCommandOff = 92,                 // REL
    InstrumentCommandGroove = 26,              // GROV
    InstrumentCommandHop = 27,                 // HOP
    InstrumentCommandRetrigger = 52,           // RTRG
    InstrumentCommandInstrumentRetrigger = 29, // IRTG
    InstrumentCommandKill = 30,                // KILL
    InstrumentCommandLegato = 31,              // LEGA
    InstrumentCommandLoopOfset = 36,           // LPOF
    InstrumentCommandMidiCC = 38,              // MDCC
    InstrumentCommandMidiPC = 39,              // MDPG
    InstrumentCommandPan = 42,                 // PAN
    InstrumentCommandPitchFineTune = 44,       // PFIN
    InstrumentCommandPlayOfset = 46,           // PLOF
    InstrumentCommandPitchSlide = 48,          // PTCH
    InstrumentCommandStop = 55,                // STOP
    InstrumentCommandTable = 58,               // TABL
    InstrumentCommandTempo = 62,               // TMPO
    InstrumentCommandVelocity = 66,            // VELM
    InstrumentCommandVolume = 69,              // VOLM
    InstrumentCommandGateLength = 28,          // TLN
    InstrumentCommandNone = 45,                // ----
    InstrumentCommandMidiChord = 143,
    InstrumentCommandRandom = 189, // RND
    Invalid = 255,
  };
  ETL_DECLARE_ENUM_TYPE(TrackerCommand, uint8_t)
  ETL_ENUM_TYPE(InstrumentCommandArpeggiator, "ARP")
  ETL_ENUM_TYPE(InstrumentCommandChance, "CHN")
  ETL_ENUM_TYPE(InstrumentCommandCrush, "CSH")
  ETL_ENUM_TYPE(InstrumentCommandKill, "KIL")
  ETL_ENUM_TYPE(InstrumentCommandLoopOfset, "LOF")
  ETL_ENUM_TYPE(InstrumentCommandVelocity, "VEL")
  ETL_ENUM_TYPE(InstrumentCommandVolume, "VOL")
  ETL_ENUM_TYPE(InstrumentCommandGateLength, "TLN")
  ETL_ENUM_TYPE(InstrumentCommandPitchSlide, "PSL")
  ETL_ENUM_TYPE(InstrumentCommandRandom, "RND")
  ETL_ENUM_TYPE(InstrumentCommandHop, "HOP")
  ETL_ENUM_TYPE(InstrumentCommandLegato, "LEG")
  ETL_ENUM_TYPE(InstrumentCommandRetrigger, "RTG")
  ETL_ENUM_TYPE(InstrumentCommandTempo, "TPO")
  ETL_ENUM_TYPE(InstrumentCommandMidiCC, "MCC")
  ETL_ENUM_TYPE(InstrumentCommandMidiPC, "MPC")
  ETL_ENUM_TYPE(InstrumentCommandPlayOfset, "POF")
  ETL_ENUM_TYPE(InstrumentCommandLowPassFilter, "FLT")
  ETL_ENUM_TYPE(InstrumentCommandTable, "TBL")
  ETL_ENUM_TYPE(InstrumentCommandFilterCut, "FCT")
  ETL_ENUM_TYPE(InstrumentCommandFilterResonance, "FRS")
  ETL_ENUM_TYPE(InstrumentCommandPan, "PAN")
  ETL_ENUM_TYPE(InstrumentCommandOff, "REL")
  ETL_ENUM_TYPE(InstrumentCommandGroove, "GRV")
  ETL_ENUM_TYPE(InstrumentCommandStop, "STP")
  ETL_ENUM_TYPE(InstrumentCommandNone, "---")
  ETL_ENUM_TYPE(InstrumentCommandPitchFineTune, "PFT")
  ETL_ENUM_TYPE(InstrumentCommandDelay, "DLY")
  ETL_ENUM_TYPE(InstrumentCommandInstrumentRetrigger, "IRT")
  ETL_ENUM_TYPE(InstrumentCommandMidiChord, "MCH")
  ETL_END_ENUM_TYPE
};

struct FourCC {
  enum enum_type : uint16_t {
    Default,

    SampleInstrumentCrushVolume,
    SampleInstrumentVolume,
    SampleInstrumentCrush,
    SampleInstrumentSample,
    SampleInstrumentInterpolation,
    SampleInstrumentDownsample,
    SampleInstrumentRootNote,
    SampleInstrumentFineTune,
    SampleInstrumentPan,
    SampleInstrumentFilterCutOff,
    SampleInstrumentFilterResonance,
    SampleInstrumentFilterType,
    SampleInstrumentFilterMode,
    SampleInstrumentStart,
    SampleInstrumentLoopMode,
    SampleInstrumentLoopStart,
    SampleInstrumentEnd,
    SampleInstrumentTable,
    SampleInstrumentTableAutomation,

    MacroInstrumentShape,
    MacroInstrmentTimbre,
    MacroInstrumentColor,
    MacroInstrumentAttack,
    MacroInstrumentDecay,
    MacroInstrumentSignature,

    MidiInstrumentChannel,
    MidiInstrumentNoteLength,
    MidiInstrumentVolume,
    MidiInstrumentTable,
    MidiInstrumentTableAutomation,
    MidiInstrumentName,
    MidiInstrumentProgram,

    SIDInstrumentWaveform,
    SIDInstrument1FilterCut,
    SIDInstrument2FilterCut,
    SIDInstrument3FilterCut,
    SIDInstrument1FilterResonance,
    SIDInstrument2FilterResonance,
    SIDInstrument3FilterResonance,
    SIDInstrument1FilterMode,
    SIDInstrument2FilterMode,
    SIDInstrument3FilterMode,
    SIDInstrument1Volume,
    SIDInstrument2Volume,
    SIDInstrument3Volume,
    SIDInstrumentPulseWidth,
    SIDInstrumentVSync,
    SIDInstrumentRingModulator,
    SIDInstrumentADSR,
    SIDInstrumentFilterOn,
    SIDInstrumentVoice3Off,
    SIDInstrumentTable,
    SIDInstrumentTableAutomation,
    SIDInstrumentOSCNumber,

    LFO1Target,
    LFO1Shape,
    LFO1Trigger,
    LFO1RateMode,
    LFO1Depth,
    LFO1HertzTenths,
    LFO1Sync,
    LFO2Target,
    LFO2Shape,
    LFO2Trigger,
    LFO2RateMode,
    LFO2Depth,
    LFO2HertzTenths,
    LFO2Sync,

    ServicePersistency,

    TrigTempoTap,
    TrigSeqQueueRow,
    TrigVolumeIncrease,
    TrigVolumeDecrease,
    TrigEventEnter,
    TrigEventEdit,
    TrigEventLeft,
    TrigEventRight,
    TrigEventUp,
    TrigEventDown,
    TrigEventAlt,
    TrigEventNav,
    TrigEventPlay,

    VarTempo,
    VarMasterVolume,
    VarPreviewVolume,
    VarWrap,
    VarTranspose,
    VarScale,
    VarScaleRoot,
    VarProjectName,
    VarProjectAudioMode,
    VarMidiDevice,
    VarMidiControlSurface,
    VarLineOut,
    VarFGColor,
    VarBGColor,
    VarHI1Color,
    VarHI2Color,
    VarConsoleColor,
    VarCursorColor,
    VarInfoColor,
    VarWarnColor,
    VarErrorColor,
    VarAccentColor,
    VarAccentAltColor,
    VarEmphasisColor,
    VarReserved1Color,
    VarReserved2Color,
    VarReserved3Color,
    VarReserved4Color,
    VarMidiSync,
    VarMidiInputDevice,
    VarMidiClockSync,
    VarRemoteUI,
    VarUIFont,

    VarChannel1Volume,
    VarChannel2Volume,
    VarChannel3Volume,
    VarChannel4Volume,
    VarChannel5Volume,
    VarChannel6Volume,
    VarChannel7Volume,
    VarChannel8Volume,
    VarChannel1FX1Send,
    VarChannel2FX1Send,
    VarChannel3FX1Send,
    VarChannel4FX1Send,
    VarChannel5FX1Send,
    VarChannel6FX1Send,
    VarChannel7FX1Send,
    VarChannel8FX1Send,
    VarChannel1FX2Send,
    VarChannel2FX2Send,
    VarChannel3FX2Send,
    VarChannel4FX2Send,
    VarChannel5FX2Send,
    VarChannel6FX2Send,
    VarChannel7FX2Send,
    VarChannel8FX2Send,
    VarChannel1FX3Send,
    VarChannel2FX3Send,
    VarChannel3FX3Send,
    VarChannel4FX3Send,
    VarChannel5FX3Send,
    VarChannel6FX3Send,
    VarChannel7FX3Send,
    VarChannel8FX3Send,
    VarFX1Volume,
    VarFX2Volume,
    VarFX3Volume,
    VarFX1Type,
    VarFX2Type,
    VarFX3Type,
    VarReverbInputGain,
    VarReverbTime,
    VarReverbDiffusion,
    VarReverbLowPass,
    VarThemeName, // Variable for storing the current theme name

    VarInstrumentType,

    ActionTempoChanged,
    ActionPurge,
    ActionPurgeInstrument,
    ActionProjectRename,
    ActionBrowse,
    ActionSave,
    ActionLoadAndSave,
    ActionCancel,
    ActionNewProject,
    ActionRandomName,
    ActionBootSelect,
    ActionEdit,
    ActionExport,
    ActionImport,
    ActionOK,
    InstrumentName,
    ActionRenderMixdown,
    ActionRenderStems,
    ActionShowTheme,
    ActionThemeName,
    ActionResetTheme,
    SampleInstrumentSlices,
    VarBacklightLevel,
    ActionShowSampleEditor,
    ActionShowSampleSlices,
    VarRecordSource,
    VarSampleEditStart,
    VarSampleEditEnd,
    VarSampleEditOperation,
    VarRecordLineGain,
    VarRecordMicGain,
    VarOutputVolume,
    VarImportResampler,
    ActionAutoSlice,
    ActionShowRecordView,
    VoiceOverdrive,
    VoiceBandpassCutoff,
    VoiceBandpassResonance,
    VoiceBandpassMix,
    VoiceFilterCutoff,
    VoiceFilterResonance,
    VoiceFilterMix,
    VoiceAmp,
    VoicePan,
    VoiceInsertType,
    VoiceAmpAttack,
    VoiceAmpDecay,
    VoiceAmpSustain,
    VoiceAmpRelease,
    VoiceFilterAttack,
    VoiceFilterDecay,
    VoiceFilterSustain,
    VoiceFilterRelease,
    VoiceFilterEnvAmount,
    VoiceInsertType2,
    VoiceInsertAmount,
    VoiceInsertAmount2,
    VoiceInsertPosition,
    VoiceInsertPosition2,
    VoiceTriggerLength,
    VoiceFilterType,
    SampleSourceInstrumentSample,
    SampleSourceInstrumentRootNote,
    SampleSourceInstrumentStart,
    SampleSourceInstrumentLoopMode,
    SampleSourceInstrumentLoopStart,
    SampleSourceInstrumentEnd,
    SampleSourceInstrumentTable,
    SampleSourceInstrumentTableAutomation,
    SampleSourceInstrumentLevel,
  };
  ETL_DECLARE_ENUM_TYPE(FourCC, uint16_t)
  // Not all enums need reflection. Only cases where we need reflection is the
  // FourCC codes that need to be converted to text in order to display on
  // screen
  ETL_ENUM_TYPE(VarLineOut, "LINEOUT")
  ETL_ENUM_TYPE(VarMidiDevice, "MIDIDEVICE")
  ETL_ENUM_TYPE(VarMidiControlSurface, "MIDICTRLSURF")
  ETL_ENUM_TYPE(VarMidiSync, "MIDISYNC")
  ETL_ENUM_TYPE(VarMidiInputDevice, "MIDIINPUTDEV")
  ETL_ENUM_TYPE(VarMidiClockSync, "MIDICLOCKSYNC")
  ETL_ENUM_TYPE(VarRemoteUI, "REMOTEUI")
  ETL_ENUM_TYPE(VarUIFont, "UIFONT")
  ETL_ENUM_TYPE(VarThemeName, "THEMENAME")
  ETL_ENUM_TYPE(VarScaleRoot, "scaleroot")
  ETL_ENUM_TYPE(MacroInstrumentShape, "shape")
  ETL_ENUM_TYPE(MacroInstrmentTimbre, "timbre")
  ETL_ENUM_TYPE(MacroInstrumentColor, "color")
  ETL_ENUM_TYPE(MacroInstrumentAttack, "Attack")
  ETL_ENUM_TYPE(MacroInstrumentDecay, "Decay")
  ETL_ENUM_TYPE(MacroInstrumentSignature, "Signature")
  ETL_ENUM_TYPE(SampleInstrumentSample, "sample")
  ETL_ENUM_TYPE(SampleInstrumentVolume, "volume")
  ETL_ENUM_TYPE(SampleInstrumentInterpolation, "interpol")
  ETL_ENUM_TYPE(SampleInstrumentCrush, "crush")
  ETL_ENUM_TYPE(SampleInstrumentCrushVolume, "crushdrive")
  ETL_ENUM_TYPE(SampleInstrumentDownsample, "downsample")
  ETL_ENUM_TYPE(SampleInstrumentRootNote, "root note")
  ETL_ENUM_TYPE(SampleInstrumentFineTune, "fine tune")
  ETL_ENUM_TYPE(SampleInstrumentPan, "pan")
  ETL_ENUM_TYPE(SampleInstrumentFilterCutOff, "filter cut")
  ETL_ENUM_TYPE(SampleInstrumentFilterResonance, "filter res")
  ETL_ENUM_TYPE(SampleInstrumentFilterType, "filter type")
  ETL_ENUM_TYPE(SampleInstrumentFilterMode, "filter mode")
  ETL_ENUM_TYPE(SampleInstrumentStart, "start")
  ETL_ENUM_TYPE(SampleInstrumentLoopMode, "loopmode")
  ETL_ENUM_TYPE(SampleInstrumentLoopStart, "loopstart")
  ETL_ENUM_TYPE(SampleInstrumentEnd, "end")
  ETL_ENUM_TYPE(SampleInstrumentTable, "table")
  ETL_ENUM_TYPE(SampleInstrumentTableAutomation, "table automation")
  ETL_ENUM_TYPE(MidiInstrumentChannel, "channel")
  ETL_ENUM_TYPE(InstrumentName, "name")
  ETL_ENUM_TYPE(MidiInstrumentName, "midi name")
  ETL_ENUM_TYPE(MidiInstrumentNoteLength, "note length")
  ETL_ENUM_TYPE(MidiInstrumentVolume, "volume")
  ETL_ENUM_TYPE(MidiInstrumentTable, "table")
  ETL_ENUM_TYPE(MidiInstrumentTableAutomation, "table automation")
  ETL_ENUM_TYPE(MidiInstrumentProgram, "program")
  ETL_ENUM_TYPE(SIDInstrumentWaveform, "VWF")
  ETL_ENUM_TYPE(SIDInstrument1FilterCut, "FILTCUT1")
  ETL_ENUM_TYPE(SIDInstrument1FilterResonance, "RES1")
  ETL_ENUM_TYPE(SIDInstrument1FilterMode, "FMODE1")
  ETL_ENUM_TYPE(SIDInstrument1Volume, "DIP_VOLUME1")
  ETL_ENUM_TYPE(SIDInstrument2FilterCut, "FILTCUT2")
  ETL_ENUM_TYPE(SIDInstrument2FilterResonance, "RES2")
  ETL_ENUM_TYPE(SIDInstrument2FilterMode, "FMODE2")
  ETL_ENUM_TYPE(SIDInstrument2Volume, "DIP_VOLUME2")
  ETL_ENUM_TYPE(SIDInstrumentPulseWidth, "VPW")
  ETL_ENUM_TYPE(SIDInstrumentVSync, "VSYNC")
  ETL_ENUM_TYPE(SIDInstrumentRingModulator, "VRING")
  ETL_ENUM_TYPE(SIDInstrumentADSR, "VADSR")
  ETL_ENUM_TYPE(SIDInstrumentFilterOn, "VFON")
  ETL_ENUM_TYPE(SIDInstrumentTable, "table")
  ETL_ENUM_TYPE(SIDInstrumentTableAutomation, "table automation")
  ETL_ENUM_TYPE(SIDInstrumentOSCNumber, "OSCNUM")

  ETL_ENUM_TYPE(LFO1Target, "lfo1 target")
  ETL_ENUM_TYPE(LFO1Shape, "lfo1 shape")
  ETL_ENUM_TYPE(LFO1Trigger, "lfo1 trigger")
  ETL_ENUM_TYPE(LFO1RateMode, "lfo1 rate mode")
  ETL_ENUM_TYPE(LFO1Depth, "lfo1 depth")
  ETL_ENUM_TYPE(LFO1HertzTenths, "lfo1 hz")
  ETL_ENUM_TYPE(LFO1Sync, "lfo1 sync")
  ETL_ENUM_TYPE(LFO2Target, "lfo2 target")
  ETL_ENUM_TYPE(LFO2Shape, "lfo2 shape")
  ETL_ENUM_TYPE(LFO2Trigger, "lfo2 trigger")
  ETL_ENUM_TYPE(LFO2RateMode, "lfo2 rate mode")
  ETL_ENUM_TYPE(LFO2Depth, "lfo2 depth")
  ETL_ENUM_TYPE(LFO2HertzTenths, "lfo2 hz")
  ETL_ENUM_TYPE(LFO2Sync, "lfo2 sync")

  ETL_ENUM_TYPE(VarFGColor, "FOREGROUND")
  ETL_ENUM_TYPE(VarBGColor, "BACKGROUND")
  ETL_ENUM_TYPE(VarHI1Color, "HICOLOR1")
  ETL_ENUM_TYPE(VarHI2Color, "HICOLOR2")
  ETL_ENUM_TYPE(VarConsoleColor, "CONSOLECOLOR")
  ETL_ENUM_TYPE(VarCursorColor, "CURSORCOLOR")
  ETL_ENUM_TYPE(VarInfoColor, "INFOCOLOR")
  ETL_ENUM_TYPE(VarWarnColor, "WARNCOLOR")
  ETL_ENUM_TYPE(VarErrorColor, "ERRORCOLOR")
  ETL_ENUM_TYPE(VarAccentColor, "ACCENTCOLOR")
  ETL_ENUM_TYPE(VarAccentAltColor, "ACCENTALTCOLOR")
  ETL_ENUM_TYPE(VarEmphasisColor, "EMPHASISCOLOR")
  ETL_ENUM_TYPE(VarReserved1Color, "RESERVED1COLOR")
  ETL_ENUM_TYPE(VarReserved2Color, "RESERVED2COLOR")
  ETL_ENUM_TYPE(VarReserved3Color, "RESERVED3COLOR")
  ETL_ENUM_TYPE(VarReserved4Color, "RESERVED4COLOR")
  ETL_ENUM_TYPE(VarTempo, "tempo")
  ETL_ENUM_TYPE(VarMasterVolume, "master")
  ETL_ENUM_TYPE(VarPreviewVolume, "preview")
  ETL_ENUM_TYPE(VarWrap, "wrap")
  ETL_ENUM_TYPE(VarTranspose, "transpose")
  ETL_ENUM_TYPE(VarScale, "scale")
  ETL_ENUM_TYPE(VarProjectName, "projectname")
  ETL_ENUM_TYPE(VarProjectAudioMode, "projectaudiomode")
  ETL_ENUM_TYPE(VarInstrumentType, "INSTRUMENTTYPE")
  ETL_ENUM_TYPE(VarChannel1Volume, "channel1vol")
  ETL_ENUM_TYPE(VarChannel2Volume, "channel2vol")
  ETL_ENUM_TYPE(VarChannel3Volume, "channel3vol")
  ETL_ENUM_TYPE(VarChannel4Volume, "channel4vol")
  ETL_ENUM_TYPE(VarChannel5Volume, "channel5vol")
  ETL_ENUM_TYPE(VarChannel6Volume, "channel6vol")
  ETL_ENUM_TYPE(VarChannel7Volume, "channel7vol")
  ETL_ENUM_TYPE(VarChannel8Volume, "channel8vol")
  ETL_ENUM_TYPE(VarChannel1FX1Send, "channel1fx1send")
  ETL_ENUM_TYPE(VarChannel2FX1Send, "channel2fx1send")
  ETL_ENUM_TYPE(VarChannel3FX1Send, "channel3fx1send")
  ETL_ENUM_TYPE(VarChannel4FX1Send, "channel4fx1send")
  ETL_ENUM_TYPE(VarChannel5FX1Send, "channel5fx1send")
  ETL_ENUM_TYPE(VarChannel6FX1Send, "channel6fx1send")
  ETL_ENUM_TYPE(VarChannel7FX1Send, "channel7fx1send")
  ETL_ENUM_TYPE(VarChannel8FX1Send, "channel8fx1send")
  ETL_ENUM_TYPE(VarChannel1FX2Send, "channel1fx2send")
  ETL_ENUM_TYPE(VarChannel2FX2Send, "channel2fx2send")
  ETL_ENUM_TYPE(VarChannel3FX2Send, "channel3fx2send")
  ETL_ENUM_TYPE(VarChannel4FX2Send, "channel4fx2send")
  ETL_ENUM_TYPE(VarChannel5FX2Send, "channel5fx2send")
  ETL_ENUM_TYPE(VarChannel6FX2Send, "channel6fx2send")
  ETL_ENUM_TYPE(VarChannel7FX2Send, "channel7fx2send")
  ETL_ENUM_TYPE(VarChannel8FX2Send, "channel8fx2send")
  ETL_ENUM_TYPE(VarChannel1FX3Send, "channel1fx3send")
  ETL_ENUM_TYPE(VarChannel2FX3Send, "channel2fx3send")
  ETL_ENUM_TYPE(VarChannel3FX3Send, "channel3fx3send")
  ETL_ENUM_TYPE(VarChannel4FX3Send, "channel4fx3send")
  ETL_ENUM_TYPE(VarChannel5FX3Send, "channel5fx3send")
  ETL_ENUM_TYPE(VarChannel6FX3Send, "channel6fx3send")
  ETL_ENUM_TYPE(VarChannel7FX3Send, "channel7fx3send")
  ETL_ENUM_TYPE(VarChannel8FX3Send, "channel8fx3send")
  ETL_ENUM_TYPE(VarFX1Volume, "fx1vol")
  ETL_ENUM_TYPE(VarFX2Volume, "fx2vol")
  ETL_ENUM_TYPE(VarFX3Volume, "fx3vol")
  ETL_ENUM_TYPE(VarFX1Type, "fx1type")
  ETL_ENUM_TYPE(VarFX2Type, "fx2type")
  ETL_ENUM_TYPE(VarFX3Type, "fx3type")
  ETL_ENUM_TYPE(VarReverbInputGain, "revin")
  ETL_ENUM_TYPE(VarReverbTime, "revtim")
  ETL_ENUM_TYPE(VarReverbDiffusion, "revdif")
  ETL_ENUM_TYPE(VarReverbLowPass, "revlp")

  ETL_ENUM_TYPE(ActionEdit, "edit")
  ETL_ENUM_TYPE(ActionExport, "export")
  ETL_ENUM_TYPE(ActionImport, "import")
  ETL_ENUM_TYPE(ActionThemeName, "themename")
  ETL_ENUM_TYPE(ActionResetTheme, "resettheme")
  ETL_ENUM_TYPE(VarBacklightLevel, "backlightLevel")
  ETL_ENUM_TYPE(VarRecordSource, "recordsource")
  ETL_ENUM_TYPE(VarRecordLineGain, "recordlinegain")
  ETL_ENUM_TYPE(VarRecordMicGain, "recordmicgain")
  ETL_ENUM_TYPE(VarOutputVolume, "outputvolume")
  ETL_ENUM_TYPE(VarImportResampler, "IMPORTRESAMP")
  ETL_ENUM_TYPE(VoiceOverdrive, "voice drive")
  ETL_ENUM_TYPE(VoiceBandpassCutoff, "voice base")
  ETL_ENUM_TYPE(VoiceBandpassResonance, "voice bw res")
  ETL_ENUM_TYPE(VoiceBandpassMix, "voice wdth")
  ETL_ENUM_TYPE(VoiceFilterCutoff, "voice filter cut")
  ETL_ENUM_TYPE(VoiceFilterResonance, "voice filter res")
  ETL_ENUM_TYPE(VoiceFilterMix, "voice filter mix")
  ETL_ENUM_TYPE(VoiceAmp, "voice amp")
  ETL_ENUM_TYPE(VoicePan, "voice pan")
  ETL_ENUM_TYPE(VoiceInsertType, "voice insert")
  ETL_ENUM_TYPE(VoiceAmpAttack, "voice amp atk")
  ETL_ENUM_TYPE(VoiceAmpDecay, "voice amp dec")
  ETL_ENUM_TYPE(VoiceAmpSustain, "voice amp sus")
  ETL_ENUM_TYPE(VoiceAmpRelease, "voice amp rel")
  ETL_ENUM_TYPE(VoiceFilterAttack, "voice filter atk")
  ETL_ENUM_TYPE(VoiceFilterDecay, "voice filter dec")
  ETL_ENUM_TYPE(VoiceFilterSustain, "voice filter sus")
  ETL_ENUM_TYPE(VoiceFilterRelease, "voice filter rel")
  ETL_ENUM_TYPE(VoiceFilterEnvAmount, "voice filter env")
  ETL_ENUM_TYPE(VoiceInsertType2, "voice insert 2")
  ETL_ENUM_TYPE(VoiceInsertAmount, "voice insert amt")
  ETL_ENUM_TYPE(VoiceInsertAmount2, "voice insert amt2")
  ETL_ENUM_TYPE(VoiceInsertPosition, "voice insert pos")
  ETL_ENUM_TYPE(VoiceInsertPosition2, "voice insert pos2")
  ETL_ENUM_TYPE(VoiceTriggerLength, "voice trig len")
  ETL_ENUM_TYPE(VoiceFilterType, "voice filter type")
  ETL_ENUM_TYPE(SampleSourceInstrumentSample, "sample")
  ETL_ENUM_TYPE(SampleSourceInstrumentRootNote, "root note")
  ETL_ENUM_TYPE(SampleSourceInstrumentStart, "start")
  ETL_ENUM_TYPE(SampleSourceInstrumentLoopMode, "loopmode")
  ETL_ENUM_TYPE(SampleSourceInstrumentLoopStart, "loopstart")
  ETL_ENUM_TYPE(SampleSourceInstrumentEnd, "end")
  ETL_ENUM_TYPE(SampleSourceInstrumentTable, "table")
  ETL_ENUM_TYPE(SampleSourceInstrumentTableAutomation, "table automation")
  ETL_ENUM_TYPE(SampleSourceInstrumentLevel, "level")

  ETL_ENUM_TYPE(Default, "   ")
  ETL_END_ENUM_TYPE
};

static_assert(sizeof(TrackerCommand) == sizeof(uint8_t),
              "Tracker commands are persisted as one byte");
static_assert(sizeof(FourCC) == sizeof(uint16_t),
              "Runtime IDs are expected to be two bytes");

static_assert(FourCC::VarChannel8Volume == FourCC::VarChannel1Volume + 7,
              "Channel volume IDs must remain contiguous");
static_assert(FourCC::VarChannel8FX3Send == FourCC::VarChannel1FX1Send + 23,
              "Channel FX send IDs must remain contiguous");
static_assert(FourCC::VarFX3Volume == FourCC::VarFX1Volume + 2,
              "FX return volume IDs must remain contiguous");
static_assert(FourCC::VarFX3Type == FourCC::VarFX1Type + 2,
              "FX type IDs must remain contiguous");
static_assert(FourCC::VarReverbLowPass == FourCC::VarReverbInputGain + 3,
              "Reverb IDs must remain contiguous");

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned char uchar;

typedef uint32_t stereosample;

#endif
