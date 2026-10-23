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

#include <cstddef>
#include <etl/array.h>
#include <etl/string.h>

static constexpr size_t HelpLegendLineCapacity = 31;

// CAUTION: all strings must fit in the line length limits!
// First line is max 31 - MAX_BATTERY_GAUGE_WIDTH, second line is max 31
// chars
using HelpLegendPair = etl::array<etl::string<HelpLegendLineCapacity>, 2>;

inline HelpLegendPair getHelpLegend(TrackerCommand command) {
  HelpLegendPair result;
  switch (command) {
  case TrackerCommand::InstrumentCommandKill:
    result[0] = "KILl: --bb";
    result[1] = "stop playing after bb ticks";
    break;
  case TrackerCommand::InstrumentCommandOff:
    result[0] = "REL: --bb";
    result[1] = "release after bb ticks";
    break;
  case TrackerCommand::InstrumentCommandLoopOfset:
    result[0] = "Loop OFset: aaaa";
    result[1] = "Shift loop start & end values aaaa";
    break;
  case TrackerCommand::InstrumentCommandArpeggiator:
    result[0] = "ARPeggio: abcd";
    result[1] = "Cycle thru relative pitch abcd";
    break;
  case TrackerCommand::InstrumentCommandChance:
    result[0] = "CHaNce: --bb";
    result[1] = "target left col; bb chance";
    break;
  case TrackerCommand::InstrumentCommandVolume:
    result[0] = "VOLume: aabb";
    result[1] = "reach volume bb at speed aa";
    break;
  case TrackerCommand::InstrumentCommandVelocity:
    result[0] = "VELocity: --bb";
    result[1] = "send MIDI velocity cmd bb";
    break;
  case TrackerCommand::InstrumentCommandPitchSlide:
    result[0] = "Pitch SLide: aabb";
    result[1] = "speed aa, slide to pitch bb";
    break;
  case TrackerCommand::InstrumentCommandHop:
    result[0] = "Hop: aabb";
    result[1] = "hop to bb aa times";
    break;
  case TrackerCommand::InstrumentCommandLegato:
    result[0] = "Legato: aaxx";
    result[1] = "slide to tied note at speed aa";
    break;
  case TrackerCommand::InstrumentCommandRetrigger:
    result[0] = "Retrigger: aabb";
    result[1] = "SAMPL:bb loop+aa ofst, MIDI:bb";
    break;
  case TrackerCommand::InstrumentCommandTempo:
    result[0] = "Tempo: aabb";
    result[1] = "set tempo to hex value aabb";
    break;
  case TrackerCommand::InstrumentCommandMidiCC:
    result[0] = "MIDI CC: aabb";
    result[1] = "CC message aa value bb";
    break;
  case TrackerCommand::InstrumentCommandMidiPC:
    result[0] = "MIDI Program Change: --bb";
    result[1] = "send program change bb";
    break;
  case TrackerCommand::InstrumentCommandPlayOfset:
    result[0] = "Play OFfset: aabb";
    result[1] = "jmp abs aa & mv rel signed bb";
    break;
  case TrackerCommand::InstrumentCommandFilterResonance:
    result[0] = "FiLTer&Res: aabb";
    result[1] = "speed aa, resonance bb";
    break;
  case TrackerCommand::InstrumentCommandLowPassFilter:
    result[0] = "FiLTeR: aabb";
    result[1] = "cutoff aa, resonance bb";
    break;
  case TrackerCommand::InstrumentCommandTable:
    result[0] = "TaBLe: --bb";
    result[1] = "run table bb";
    break;
  case TrackerCommand::InstrumentCommandGateLength:
    result[0] = "Trig LENgth: --bb";
    result[1] = "stop/rel after bb ticks";
    break;
  case TrackerCommand::InstrumentCommandCrush:
    result[0] = "drive&CruSH: aa-b";
    result[1] = "drive aa crush -b";
    break;
  case TrackerCommand::InstrumentCommandFilterCut:
    result[0] = "FilterCuToff: aabb";
    result[1] = "speed aa, target cutoff bb";
    break;
  case TrackerCommand::InstrumentCommandPan:
    result[0] = "PAN: aabb";
    result[1] = "speed aa, value bb (00 right)";
    break;
  case TrackerCommand::InstrumentCommandGroove:
    result[0] = "GRooVe: aabb";
    result[1] = "set bb (aa > 0,set all tracks)";
    break;
  case TrackerCommand::InstrumentCommandInstrumentRetrigger:
    result[0] = "InstrumentReTrig: --bb";
    result[1] = "retrigger & transpose by bb";
    break;
  case TrackerCommand::InstrumentCommandPitchFineTune:
    result[0] = "PitchFineTune: aabb";
    result[1] = "speed aa, tune bb (~+/-1 st)";
    break;
  case TrackerCommand::InstrumentCommandDelay:
    result[0] = "Delay: ---b";
    result[1] = "delay b+1 ticks";
    break;
  case TrackerCommand::InstrumentCommandDelayTime:
    result[0] = "DeLay Time: --bb";
    result[1] = "set delay TIM to bb ticks";
    break;
  case TrackerCommand::InstrumentCommandDelayFeedback:
    result[0] = "DeLay Feedback: --bb";
    result[1] = "set delay FDB to bb";
    break;
  case TrackerCommand::InstrumentCommandReverbPreDelay:
    result[0] = "ReVerb Pre-delay: --bb";
    result[1] = "set reverb pre-delay to bb";
    break;
  case TrackerCommand::InstrumentCommandReverbInputTone:
    result[0] = "ReVerb Input tone: --bb";
    result[1] = "set reverb input tone to bb";
    break;
  case TrackerCommand::InstrumentCommandReverbDiffuse:
    result[0] = "ReVerb diFfuse: --bb";
    result[1] = "set reverb diffusion to bb";
    break;
  case TrackerCommand::InstrumentCommandReverbDecay:
    result[0] = "ReVerb Decay: --bb";
    result[1] = "set decay; FF holds tail";
    break;
  case TrackerCommand::InstrumentCommandReverbTailTone:
    result[0] = "ReVerb Tail tone: --bb";
    result[1] = "set reverb tail tone to bb";
    break;
  case TrackerCommand::InstrumentCommandReverbModulationSpeed:
    result[0] = "ReVerb mod Speed: --bb";
    result[1] = "set reverb movement speed";
    break;
  case TrackerCommand::InstrumentCommandReverbModulationLevel:
    result[0] = "ReVerb mod Level: --bb";
    result[1] = "set reverb movement level";
    break;
  case TrackerCommand::InstrumentCommandRandom:
    result[0] = "RaNDom: --bb";
    result[1] = "target left col; bb depth";
    break;
  case TrackerCommand::InstrumentCommandStop:
    result[0] = "Stop table playback";
    result[1] = "";
    break;
  case TrackerCommand::InstrumentCommandMidiChord:
    result[0] = "MIDI Chord:abcd";
    result[1] = "send rel notes:+a,+b,+c,+d";
    break;
  default:
    result[0] = "";
    result[1] = "";
    break;
  }
  return result;
}
