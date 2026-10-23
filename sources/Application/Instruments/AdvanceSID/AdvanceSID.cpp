/*
 * SPDX-License-Identifier: LicenseRef-cRSID-WTF AND GPL-3.0-only
 *
 * Copyright (c) 2022 Mihaly Horvath (original cRSID implementation)
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Derived from cRSID 1.1 by Hermit (Mihaly Horvath).
 * Original cRSID terms are preserved in
 * LICENSES/LicenseRef-cRSID-WTF.txt at the repository root.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Existing license grants are preserved. See LICENSING.md.
 */

// AdvanceSID SID emulation engine
#include "AdvanceSID.h"
#include "AdvanceSIDFilter.h"
#include "AdvanceSIDWaves.h"
#include "config/MemorySections.h"
#include <string.h>

namespace {
constexpr int16_t kADSRPrescalePeriods[16] = {
    9,   32,  63,   95,   149,  220,   267,   313,
    392, 977, 1954, 3126, 3907, 11720, 19532, 31251};

constexpr uint8_t kADSRExponentPeriods[256] = {
    1, 30, 30, 30, 30, 30, 30, 16, 16, 16, 16, 16, 16, 16, 16, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, // pos0:1  pos6:30  pos14:16
                                     // pos26:8
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, // pos54:4 //pos93:2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

constexpr uint8_t kADSRGateBit = 0x01;
constexpr uint8_t kADSRAttackBit = 0x80;
constexpr uint8_t kADSRDecaySustainBit = 0x40;
constexpr uint8_t kADSRHoldZeroNotBit = 0x10;
constexpr uint16_t kADSRRateCounterPeriod = 0x8000;
} // namespace

int AdvanceSID::EmulateOutputStage(const RenderParameters &parameters) {
  enum SIDspecs { CHANNELS = 3 + 1, VOLUME_MAX = 0xF };
  enum FilterBits {
    HIGHPASS_BITVAL = 0x40,
    BANDPASS_BITVAL = 0x20,
    LOWPASS_BITVAL = 0x10
  };

  int Tmp, NonFilted, FilterInput, FilterOutput, Output;

  NonFilted = NonFiltedSample;
  FilterInput = FilterInputSample;

  // Filter
  int32_t cutoff = parameters.cutoff;
  if (model_ == Model::MOS6581) {
    int32_t distortedCutoff =
        parameters.filterCutoff + ((FilterInput * 105) >> 16);
    if (distortedCutoff > 0x7FF)
      distortedCutoff = 0x7FF;
    else if (distortedCutoff < 0)
      distortedCutoff = 0; // MOSFET-VCR control-voltage calculation
    cutoff = CutoffMul6581_44100Hz[distortedCutoff];
    // This input-dependent resistance modulation recreates the characteristic
    // nonlinear distortion of the 6581 filter.
  }

  FilterOutput = 0;
  Tmp =
      FilterInput + ((PrevBandPass * parameters.resonance) >> 12) + PrevLowPass;
  if (parameters.volumeBand & HIGHPASS_BITVAL)
    FilterOutput -= Tmp;
  Tmp = PrevBandPass - ((Tmp * cutoff) >> 12);
  PrevBandPass = Tmp;
  if (parameters.volumeBand & BANDPASS_BITVAL)
    FilterOutput -= Tmp;
  Tmp = PrevLowPass + ((Tmp * cutoff) >> 12);
  PrevLowPass = Tmp;
  if (parameters.volumeBand & LOWPASS_BITVAL)
    FilterOutput += Tmp;

  // Output-mixing stage
  Output = ((NonFilted + FilterOutput) * parameters.mainVolume) /
           ((CHANNELS * VOLUME_MAX) + ATTENUATION);
  return Output; // master output of a SID
}

AdvanceSID::AdvanceSID(uint16_t samplerate)
    : AdvanceSID(Model::MOS8580, samplerate) {}

AdvanceSID::AdvanceSID(Model model, uint16_t samplerate)
    : model_(model), adsrCyclesPerSample_(C64_PAL_CPUCLK / samplerate),
      sampleRate_(samplerate),
      adsrClockRemainderPerSample_(C64_PAL_CPUCLK % samplerate),
      adsrClockRemainder_(0) {
  SampleClockRatio = (C64_PAL_CPUCLK << 4) / samplerate;
  Reset();
}

void AdvanceSID::SetModel(Model model) {
  if (model_ == model)
    return;

  model_ = model;
  // The filter, envelopes, and combined-waveform state are model-dependent.
  // Clear them when changing chips so no state from the old model leaks into
  // the first note rendered by the new one.
  Reset();
}

void AdvanceSID::Reset() {
  memset(Register, 0, sizeof(Register));
  memset(ADSRstate, 0, sizeof(ADSRstate));
  memset(RateCounter, 0, sizeof(RateCounter));
  memset(EnvelopeCounter, 0, sizeof(EnvelopeCounter));
  memset(ExponentCounter, 0, sizeof(ExponentCounter));
  memset(PhaseAccu, 0, sizeof(PhaseAccu));
  memset(PrevPhaseAccu, 0, sizeof(PrevPhaseAccu));
  memset(NoiseLFSR, 0, sizeof(NoiseLFSR));
  memset(PrevWavGenOut, 0, sizeof(PrevWavGenOut));
  memset(PrevWavData, 0, sizeof(PrevWavData));

  unsigned char Channel;
  for (Channel = 0; Channel < 21; Channel += 7) {
    NoiseLFSR[Channel] = 0x7FFFFF;
  }
  SyncSourceMSBrise = 0;
  RingSourceMSB = 0;
  PrevLowPass = PrevBandPass = 0;
  NonFiltedSample = FilterInputSample = 0;
  adsrClockRemainder_ = 0;
}

void AdvanceSID::EmulateADSRs() { EmulateADSRs(NextADSRCycleCount()); }

void AdvanceSID::EmulateADSRs(uint32_t cycles) {
  ADSRRenderParameters parameters;
  PrecalculateADSRParameters(parameters);
  AdvanceADSRs(cycles, parameters);
}

PICOTRACKER_FAST_CODE("advance_sid_voices_idle")
bool AdvanceSID::AreVoicesIdle() const {
  for (uint8_t voiceIndex = 0; voiceIndex < 3; ++voiceIndex) {
    const uint8_t channel = voiceIndex * 7;
    if ((Register[channel + 4] & kADSRGateBit) != 0 ||
        EnvelopeCounter[channel] != 0) {
      return false;
    }
  }
  return true;
}

PICOTRACKER_FAST_CODE("advance_sid_next_adsr_cycle")
uint32_t AdvanceSID::NextADSRCycleCount() {
  uint32_t cycles = adsrCyclesPerSample_;
  uint32_t remainder = adsrClockRemainder_ + adsrClockRemainderPerSample_;
  if (remainder >= sampleRate_) {
    remainder -= sampleRate_;
    ++cycles;
  }
  adsrClockRemainder_ = static_cast<uint16_t>(remainder);
  return cycles;
}

__attribute__((always_inline)) inline void AdvanceSID::AdvanceADSRVoice(
    uint32_t cycles, const ADSRVoiceRenderParameters &voice, uint8_t &state,
    uint16_t &rateCounter, uint8_t &envelope, uint8_t &exponentCounter) {
  const uint8_t previousGate = state & kADSRGateBit;
  if (previousGate != voice.gate) { // gatebit-change?
    if (previousGate)
      state &= ~(kADSRGateBit | kADSRAttackBit |
                 kADSRDecaySustainBit); // falling edge
    else
      state = (kADSRGateBit | kADSRAttackBit | kADSRDecaySustainBit |
               kADSRHoldZeroNotBit); // rising edge
  }

  while (cycles > 0) {
    uint16_t prescalePeriod;
    if (state & kADSRAttackBit)
      prescalePeriod = voice.attackPeriod;
    else if (state & kADSRDecaySustainBit)
      prescalePeriod = voice.decayPeriod;
    else
      prescalePeriod = voice.releasePeriod;

    // If a rate change selected a period already passed by the counter, the
    // genuine SID does not match it until the 15-bit rate counter wraps. The
    // genuine counter is an LFSR; this preserves the existing linear-counter
    // approximation of that ADSR delay bug.
    const uint32_t cyclesToRateEvent =
        rateCounter < prescalePeriod
            ? prescalePeriod - rateCounter
            : kADSRRateCounterPeriod - rateCounter + prescalePeriod;
    if (cycles < cyclesToRateEvent) {
      rateCounter = static_cast<uint16_t>((rateCounter + cycles) &
                                          (kADSRRateCounterPeriod - 1));
      break;
    }

    cycles -= cyclesToRateEvent;
    rateCounter = 0; // reset rate-counter on period-match
    const bool envelopeStep =
        (state & kADSRAttackBit) ||
        ++exponentCounter == kADSRExponentPeriods[envelope];
    if (!envelopeStep)
      continue;

    exponentCounter = 0;
    if (state & kADSRHoldZeroNotBit) {
      if (state & kADSRAttackBit) {
        ++envelope;
        if (envelope == 0xFF)
          state &= ~kADSRAttackBit;
      } else if (!(state & kADSRDecaySustainBit) ||
                 envelope != voice.sustainLevel) {
        --envelope; // resid adds 1 cycle delay, we omit that mechanism here
        if (state & kADSRDecaySustainBit) {
          if (envelope < voice.sustainLevel)
            envelope = voice.sustainLevel;
        } else if (envelope == 0)
          state &= ~kADSRHoldZeroNotBit;
      }
    }
  }
}

void AdvanceSID::AdvanceADSRs(uint32_t cycles,
                              const ADSRRenderParameters &parameters) {
  for (uint8_t voiceIndex = 0; voiceIndex < 3; ++voiceIndex) {
    const uint8_t channel = voiceIndex * 7;
    AdvanceADSRVoice(cycles, parameters.voices[voiceIndex], ADSRstate[channel],
                     RateCounter[channel], EnvelopeCounter[channel],
                     ExponentCounter[channel]);
  }
}

PICOTRACKER_FAST_CODE("advance_sid_adsr_chunk")
__attribute__((noinline)) void
AdvanceSID::EmulateADSRChunk(const ADSRRenderParameters &parameters,
                             RenderChunk &chunk, uint8_t sampleCount) {
  uint32_t sampleCycles[RENDER_CHUNK_SIZE];
  for (uint8_t sample = 0; sample < sampleCount; ++sample)
    sampleCycles[sample] = NextADSRCycleCount();

  for (uint8_t voiceIndex = 0; voiceIndex < 3; ++voiceIndex) {
    const ADSRVoiceRenderParameters &voice = parameters.voices[voiceIndex];
    const uint8_t channel = voiceIndex * 7;
    uint8_t state = ADSRstate[channel];
    uint16_t rateCounter = RateCounter[channel];
    uint8_t envelope = EnvelopeCounter[channel];
    uint8_t exponentCounter = ExponentCounter[channel];

    for (uint8_t sample = 0; sample < sampleCount; ++sample) {
      AdvanceADSRVoice(sampleCycles[sample], voice, state, rateCounter,
                       envelope, exponentCounter);
      chunk.envelopes[voiceIndex][sample] = envelope;
    }

    ADSRstate[channel] = state;
    RateCounter[channel] = rateCounter;
    EnvelopeCounter[channel] = envelope;
    ExponentCounter[channel] = exponentCounter;
  }
}

PICOTRACKER_FAST_CODE("advance_sid_prepare_adsr")
void AdvanceSID::PrecalculateADSRParameters(ADSRRenderParameters &parameters) {
  unsigned char Channel, Voice;
  for (Channel = 0, Voice = 0; Channel < 21; Channel += 7, Voice++) {
    ADSRVoiceRenderParameters &voice = parameters.voices[Voice];
    const uint8_t AD = Register[Channel + 5];
    const uint8_t SR = Register[Channel + 6];

    voice.attackPeriod = kADSRPrescalePeriods[AD >> 4];
    voice.decayPeriod = kADSRPrescalePeriods[AD & 0x0F];
    voice.releasePeriod = kADSRPrescalePeriods[SR & 0x0F];
    voice.sustainLevel = (SR & 0xF0) + (SR >> 4);
    voice.gate = Register[Channel + 4] & 0x01;
  }
}

void AdvanceSID::ResetChannel(unsigned char channel) {
  ADSRstate[channel] = 0;
  RateCounter[channel] = 0;
  EnvelopeCounter[channel] = 0;
  ExponentCounter[channel] = 0;
  PhaseAccu[channel] = 0;
  PrevPhaseAccu[channel] = 0;
  NoiseLFSR[channel] = 0x7FFFFF;
  PrevWavGenOut[channel] = 0;
  PrevWavData[channel] = 0;
}

__attribute__((always_inline)) inline unsigned short
AdvanceSID::combinedWF(const unsigned char *WFarray, unsigned short oscval,
                       unsigned char Channel, unsigned short Filt) {
  if (model_ == Model::MOS6581 && WFarray != PulseTriangle)
    oscval &= 0x7FFF;
  const int32_t previous = PrevWavData[Channel];
  const int32_t target = WFarray[oscval >> 4];
  PrevWavData[Channel] =
      previous + (((target - previous) * Filt - previous) >> 16);
  return PrevWavData[Channel] << 8;
}

__attribute__((always_inline)) inline unsigned short
AdvanceSID::combinedWFChunk(const unsigned char *WFarray, unsigned short oscval,
                            unsigned short Filt, uint8_t &previousWaveData) {
  if (model_ == Model::MOS6581 && WFarray != PulseTriangle)
    oscval &= 0x7FFF;
  const int32_t previous = previousWaveData;
  const int32_t target = WFarray[oscval >> 4];
  previousWaveData = previous + (((target - previous) * Filt - previous) >> 16);
  return previousWaveData << 8;
}

PICOTRACKER_FAST_CODE("advance_sid_prepare_render")
void AdvanceSID::PrecalculateRenderParameters(RenderParameters &parameters) {
  PrecalculateADSRParameters(parameters.adsr);

  parameters.filterSwitchReso = Register[0x17];
  parameters.volumeBand = Register[0x18];

  unsigned char Channel, Voice;
  for (Channel = 0, Voice = 0; Channel < 21; Channel += 7, Voice++) {
    VoiceRenderParameters &voice = parameters.voices[Voice];
    voice.waveform = Register[Channel + 4];
    if (parameters.filterSwitchReso & (1 << Voice)) {
      voice.route = VOICE_ROUTE_FILTER;
    } else if (Voice != 2 || !(parameters.volumeBand & 0x80)) {
      voice.route = VOICE_ROUTE_OUTPUT;
    } else {
      voice.route = VOICE_ROUTE_DISABLED;
    }
    voice.phaseAccuStep =
        ((Register[Channel + 1] << 8) + Register[Channel]) * SampleClockRatio;

    voice.pulseWidth =
        (((Register[Channel + 3] & 0xF) << 8) + Register[Channel + 2]) << 4;
    unsigned int minimumPulseWidth = voice.phaseAccuStep >> 13;
    if (0 < voice.pulseWidth && voice.pulseWidth < minimumPulseWidth)
      voice.pulseWidth = minimumPulseWidth; // Too thin pulsewidth? Correct...
    unsigned int maximumPulseWidth = minimumPulseWidth ^ 0xFFFF;
    if (voice.pulseWidth > maximumPulseWidth)
      voice.pulseWidth =
          maximumPulseWidth; // Too thin pulsewidth? Correct it to a value
                             // representable at the current samplerate

    voice.pulseSteepness =
        (voice.phaseAccuStep >= 4096)
            ? 0xFFFFFFF / voice.phaseAccuStep
            : 0xFFFF; // rising/falling-edge steepness (add/sub at samples)
    voice.pulseRisingPeak = (0xFFFF - voice.pulseWidth) * voice.pulseSteepness;
    if (voice.pulseRisingPeak > 0xFFFF)
      voice.pulseRisingPeak = 0xFFFF; // but adequately thick trapezoid pulses
                                      // reach the maximum level
    voice.pulseFallingPeak = voice.pulseWidth * voice.pulseSteepness;
    if (voice.pulseFallingPeak > 0xFFFF)
      voice.pulseFallingPeak =
          0xFFFF; // adequately thick trapezoid pulses reach the maximum level

    voice.sawSteepness = (voice.phaseAccuStep >> 4) / 288;
    if (voice.sawSteepness == 0)
      voice.sawSteepness = 1; // avoid division by zero in next steps

    unsigned char Pitch = Register[Channel + 1] ? Register[Channel + 1]
                                                : 1; // avoid division by zero
    voice.combinedWaveformFilter = 0x7777 + (0x8888 / Pitch);
  }

  parameters.filterCutoff = (Register[0x16] << 3) + (Register[0x15] & 7);
  if (model_ == Model::MOS8580) {
    parameters.cutoff = CutoffMul8580_44100Hz[parameters.filterCutoff];
    parameters.resonance = Resonances8580[parameters.filterSwitchReso >> 4];
  } else {
    // The 6581 cutoff is calculated per sample because the input signal
    // modulates it in EmulateOutputStage().
    parameters.cutoff = 0;
    parameters.resonance = Resonances6581[parameters.filterSwitchReso >> 4];
  }
  parameters.mainVolume = parameters.volumeBand & 0xF;
}

PICOTRACKER_FAST_CODE("advance_sid_wave_chunk")
__attribute__((noinline)) void
AdvanceSID::EmulateWaveGeneratorsChunk(const RenderParameters &parameters,
                                       RenderChunk &chunk,
                                       uint8_t sampleCount) {
  enum WaveFormBits {
    NOISE_BITVAL = 0x80,
    PULSE_BITVAL = 0x40,
    SAW_BITVAL = 0x20,
    TRI_BITVAL = 0x10
  };
  enum ControlBits {
    TEST_BITVAL = 0x08,
    RING_BITVAL = 0x04,
    SYNC_BITVAL = 0x02,
    GATE_BITVAL = 0x01
  };

  uint32_t phaseAccumulators[3] = {static_cast<uint32_t>(PhaseAccu[0]),
                                   static_cast<uint32_t>(PhaseAccu[7]),
                                   static_cast<uint32_t>(PhaseAccu[14])};
  uint32_t previousPhaseAccumulators[3] = {
      static_cast<uint32_t>(PrevPhaseAccu[0]),
      static_cast<uint32_t>(PrevPhaseAccu[7]),
      static_cast<uint32_t>(PrevPhaseAccu[14])};
  uint8_t syncSourceMSBrise = SyncSourceMSBrise;
  uint32_t ringSourceMSB = RingSourceMSB;

  // Advance phases in the original sample/voice order because oscillator sync
  // consumes the immediately preceding voice's MSB transition.
  for (uint8_t sample = 0; sample < sampleCount; sample++) {
    for (uint8_t Voice = 0; Voice < 3; Voice++) {
      const VoiceRenderParameters &voice = parameters.voices[Voice];
      const uint8_t waveform = voice.waveform;
      const bool testBit = (waveform & TEST_BITVAL) != 0;
      uint32_t &phaseAccumulator = phaseAccumulators[Voice];

      chunk.ringSourceMSBs[Voice][sample] = ringSourceMSB != 0;
      if (testBit || ((waveform & SYNC_BITVAL) && syncSourceMSBrise))
        phaseAccumulator = 0;
      else { // stepping phase-accumulator (oscillator)
        phaseAccumulator += voice.phaseAccuStep;
        if (phaseAccumulator >= 0x10000000)
          phaseAccumulator -= 0x10000000;
      }
      phaseAccumulator &= 0xFFFFFFF;
      chunk.phases[Voice][sample] = phaseAccumulator;

      const uint32_t MSB = phaseAccumulator & 0x8000000;
      syncSourceMSBrise = MSB > (previousPhaseAccumulators[Voice] & 0x8000000);
      previousPhaseAccumulators[Voice] = phaseAccumulator;
      ringSourceMSB = MSB;
    }
  }

  PhaseAccu[0] = phaseAccumulators[0];
  PhaseAccu[7] = phaseAccumulators[1];
  PhaseAccu[14] = phaseAccumulators[2];
  SyncSourceMSBrise = syncSourceMSBrise;
  RingSourceMSB = ringSourceMSB;

  for (uint8_t sample = 0; sample < sampleCount; sample++) {
    chunk.nonFiltedSamples[sample] = 0;
    chunk.filterInputSamples[sample] = 0;
  }

  // Waveforms are fixed for a render call, so dispatch once per voice and keep
  // its state local across the chunk.
  uint8_t Channel, Voice;
  for (Channel = 0, Voice = 0; Channel < 21; Channel += 7, Voice++) {
    const VoiceRenderParameters &voice = parameters.voices[Voice];
    const uint8_t waveform = voice.waveform;
    const bool testBit = (waveform & TEST_BITVAL) != 0;
    uint32_t noiseLFSR = NoiseLFSR[Channel];
    uint32_t previousWaveGeneratorOutput = PrevWavGenOut[Channel];
    uint8_t previousWaveData = PrevWavData[Channel];
    uint16_t waveOutputs[RENDER_CHUNK_SIZE];

    if (waveform & NOISE_BITVAL) { // noise waveform
      for (uint8_t sample = 0; sample < sampleCount; sample++) {
        const uint32_t phaseAccumulator = chunk.phases[Voice][sample];
        const uint32_t previousPhaseAccumulator =
            sample == 0 ? static_cast<uint32_t>(PrevPhaseAccu[Channel])
                        : chunk.phases[Voice][sample - 1];
        uint32_t Tmp = noiseLFSR; // clock LFSR all time if clockrate exceeds
                                  // observable at given samplerate (last term):
        if (((phaseAccumulator & 0x1000000) !=
             (previousPhaseAccumulator & 0x1000000)) ||
            voice.phaseAccuStep >= 0x1000000) {
          const uint32_t Feedback =
              ((Tmp & 0x400000) ^ ((Tmp & 0x20000) << 5)) != 0;
          Tmp = ((Tmp << 1) | Feedback | testBit) &
                0x7FFFFF; // TEST-bit turns all bits in noise LFSR to 1 (on real
                          // SID slowly, in approx. 8000 microseconds ~ 300
                          // samples)
          noiseLFSR = Tmp;
        } // we simply zero output when other waveform is mixed with noise. On
          // real SID LFSR continuously gets filled by zero and locks up. ($C1
          // waveform with pw<8 can keep it for a while.)
        const uint32_t waveGeneratorOutput =
            (waveform & 0x70)
                ? 0
                : ((Tmp & 0x100000) >> 5) | ((Tmp & 0x40000) >> 4) |
                      ((Tmp & 0x4000) >> 1) | ((Tmp & 0x800) << 1) |
                      ((Tmp & 0x200) << 2) | ((Tmp & 0x20) << 5) |
                      ((Tmp & 0x04) << 7) | ((Tmp & 0x01) << 8);
        waveOutputs[sample] = waveGeneratorOutput & 0xFFFF;
      }
    } else if (waveform & PULSE_BITVAL) { // simple pulse or combined pulse
      const uint32_t pulseWidth =
          voice.pulseWidth; // PW=0000..FFF0 from SID-register
      if ((waveform & 0xF0) == PULSE_BITVAL) {
        const int32_t steepness = voice.pulseSteepness;
        for (uint8_t sample = 0; sample < sampleCount; sample++) {
          const uint32_t phase = chunk.phases[Voice][sample] >> 12;
          uint32_t waveGeneratorOutput;
          if (testBit)
            waveGeneratorOutput = 0xFFFF;
          else if (phase < pulseWidth) { // rising edge (interpolation)
            const int32_t pulsePeak =
                voice.pulseRisingPeak; // very thin pulses don't make a full
                                       // swing between 0 and max but make a
                                       // little spike
            const int32_t Tmp =
                pulsePeak -
                (pulseWidth - phase) * steepness;    // draw slope from the peak
            waveGeneratorOutput = Tmp < 0 ? 0 : Tmp; // but stop at 0-level
          } else { // falling edge (interpolation)
            const int32_t pulsePeak =
                voice.pulseFallingPeak; // very thin pulses don't make a full
                                        // swing between 0 and max but make a
                                        // little spike
            const int32_t Tmp = (0xFFFF - phase) * steepness -
                                pulsePeak; // draw the slope from the peak
            waveGeneratorOutput =
                Tmp >= 0 ? 0xFFFF : Tmp; // but stop at max-level
          }
          waveOutputs[sample] = waveGeneratorOutput & 0xFFFF;
        }
      } else { // combined pulse
        for (uint8_t sample = 0; sample < sampleCount; sample++) {
          const uint32_t phaseAccumulator = chunk.phases[Voice][sample];
          const uint32_t phase = phaseAccumulator >> 12;
          uint32_t waveGeneratorOutput =
              (phase >= pulseWidth || testBit) ? 0xFFFF : 0;
          if (waveform & TRI_BITVAL) {
            if (waveform & SAW_BITVAL) { // pulse+saw+triangle (waveform nearly
                                         // identical to tri+saw)
              if (waveGeneratorOutput)
                waveGeneratorOutput = combinedWFChunk(
                    PulseSawTriangle, phase, voice.combinedWaveformFilter,
                    previousWaveData);
            } else { // pulse+triangle
              const uint32_t ringSource =
                  chunk.ringSourceMSBs[Voice][sample] ? 0x8000000 : 0;
              const uint32_t Tmp = phaseAccumulator ^
                                   ((waveform & RING_BITVAL) ? ringSource : 0);
              if (waveGeneratorOutput)
                waveGeneratorOutput = combinedWFChunk(
                    PulseTriangle, Tmp >> 12, voice.combinedWaveformFilter,
                    previousWaveData);
            }
          } else if (waveform & SAW_BITVAL) { // pulse+saw
            if (waveGeneratorOutput)
              waveGeneratorOutput = combinedWFChunk(
                  PulseSawtooth, phase, voice.combinedWaveformFilter,
                  previousWaveData);
          }
          waveOutputs[sample] = waveGeneratorOutput & 0xFFFF;
        }
      }
    } else if (waveform & SAW_BITVAL) { // sawtooth
      for (uint8_t sample = 0; sample < sampleCount; sample++) {
        uint32_t waveGeneratorOutput =
            chunk.phases[Voice][sample] >>
            12; // saw (this row would be enough for simple but
                // aliased-at-high-pitch saw)
        if (waveform & TRI_BITVAL)
          waveGeneratorOutput = combinedWFChunk(
              SawTriangle, waveGeneratorOutput, voice.combinedWaveformFilter,
              previousWaveData); // saw+triangle
        else {                   // simple cleaned (bandlimited) saw
          const int32_t steepness = voice.sawSteepness;
          waveGeneratorOutput += (waveGeneratorOutput * steepness) >>
                                 16; // 1st half (rising edge) of asymmetric
                                     // triangle-like saw waveform
          if (waveGeneratorOutput > 0xFFFF)
            waveGeneratorOutput =
                0xFFFF -
                (((waveGeneratorOutput - 0x10000) << 16) /
                 steepness); // 2nd half (falling edge, reciprocal steepness)
        }
        waveOutputs[sample] = waveGeneratorOutput & 0xFFFF;
      }
    } else if (waveform & TRI_BITVAL) { // triangle (this waveform has no harsh
                                        // edges, so it doesn't suffer from
                                        // strong aliasing at high pitches)
      for (uint8_t sample = 0; sample < sampleCount; sample++) {
        const uint32_t ringSource =
            chunk.ringSourceMSBs[Voice][sample] ? 0x8000000 : 0;
        const int32_t Tmp = chunk.phases[Voice][sample] ^
                            ((waveform & RING_BITVAL) ? ringSource : 0);
        const uint32_t waveGeneratorOutput =
            (Tmp ^ (Tmp & 0x8000000 ? 0xFFFFFFF : 0)) >> 11;
        waveOutputs[sample] = waveGeneratorOutput & 0xFFFF;
      }
    } else {
      for (uint8_t sample = 0; sample < sampleCount; sample++)
        waveOutputs[sample] = previousWaveGeneratorOutput;
    }

    if (waveform & 0xF0)
      previousWaveGeneratorOutput = waveOutputs[sampleCount - 1];
    NoiseLFSR[Channel] = noiseLFSR;
    PrevWavGenOut[Channel] = previousWaveGeneratorOutput;
    PrevWavData[Channel] = previousWaveData;
    PrevPhaseAccu[Channel] = chunk.phases[Voice][sampleCount - 1];

    // routing the channel signal to either the filter or the unfiltered master
    // output depending on filter-switch SID-registers
    if (voice.route == VOICE_ROUTE_FILTER) {
      for (uint8_t sample = 0; sample < sampleCount; sample++) {
        const uint8_t envelope =
            model_ == Model::MOS6581
                ? ADSR_DAC_6581[chunk.envelopes[Voice][sample]]
                : chunk.envelopes[Voice][sample];
        chunk.filterInputSamples[sample] +=
            ((static_cast<int32_t>(waveOutputs[sample]) - 0x8000) * envelope) >>
            8;
      }
    } else if (voice.route == VOICE_ROUTE_OUTPUT) {
      for (uint8_t sample = 0; sample < sampleCount; sample++) {
        const uint8_t envelope =
            model_ == Model::MOS6581
                ? ADSR_DAC_6581[chunk.envelopes[Voice][sample]]
                : chunk.envelopes[Voice][sample];
        chunk.nonFiltedSamples[sample] +=
            ((static_cast<int32_t>(waveOutputs[sample]) - 0x8000) * envelope) >>
            8;
      }
    }
  }
  // update readable SID1-registers (some SID tunes might use 3rd channel
  // ENV3/OSC3 value as control)
  //  SID->C64->IObankRD[SID->BaseAddress+0x1B] = WavGenOut>>8; //OSC3, ENV3
  //  (some players rely on it, unfortunately even for timing)
  //  SID->C64->IObankRD[SID->BaseAddress+0x1C] = SID->EnvelopeCounter[14];
  //  //Envelope
}

PICOTRACKER_FAST_CODE("advance_sid_output_chunk")
__attribute__((noinline)) void
AdvanceSID::EmulateOutputChunk(const RenderParameters &parameters,
                               const RenderChunk &chunk, AudioSample *buffer,
                               uint8_t sampleCount) {
  for (uint8_t sample = 0; sample < sampleCount; sample++) {
    NonFiltedSample = chunk.nonFiltedSamples[sample];
    FilterInputSample = chunk.filterInputSamples[sample];
    const int output = EmulateOutputStage(parameters);
    buffer[sample] =
        static_cast<AudioSample>(output) / kAudioPcmScale; // packed mono
  }
}

PICOTRACKER_FAST_CODE("advance_sid_render")
void AdvanceSID::Render(AudioSample *buffer, int size) {
  RenderParameters parameters;
  PrecalculateRenderParameters(parameters);
  RenderChunk chunk;
  for (int offset = 0; offset < size; offset += RENDER_CHUNK_SIZE) {
    const uint8_t sampleCount = static_cast<uint8_t>(
        (size - offset) < RENDER_CHUNK_SIZE ? size - offset
                                            : RENDER_CHUNK_SIZE);
    EmulateADSRChunk(parameters.adsr, chunk, sampleCount);
    EmulateWaveGeneratorsChunk(parameters, chunk, sampleCount);
    EmulateOutputChunk(parameters, chunk, &buffer[offset], sampleCount);
  }
}

int AdvanceSID::EmulateWaves() {
  RenderParameters parameters;
  PrecalculateRenderParameters(parameters);
  return EmulateWaves(parameters);
}

__attribute__((always_inline)) inline int
AdvanceSID::EmulateWaves(const RenderParameters &parameters) {
  EmulateWaveGenerators(parameters);
  return EmulateOutputStage(parameters);
}

__attribute__((always_inline)) inline void
AdvanceSID::EmulateWaveGenerators(const RenderParameters &parameters) {
  enum WaveFormBits {
    NOISE_BITVAL = 0x80,
    PULSE_BITVAL = 0x40,
    SAW_BITVAL = 0x20,
    TRI_BITVAL = 0x10
  };
  enum ControlBits {
    TEST_BITVAL = 0x08,
    RING_BITVAL = 0x04,
    SYNC_BITVAL = 0x02,
    GATE_BITVAL = 0x01
  };
  unsigned char Channel, Voice, WF, TestBit, Envelope;
  unsigned int Utmp, PhaseAccuStep, MSB, WavGenOut, PW;
  int Tmp, Feedback, Steepness, PulsePeak;
  // static int FilterInput, Cutoff, Resonance, FilterOutput, NonFilted, Output;
  int *PhaseAccuPtr;

  int nonFiltedSample = 0;
  int filterInputSample = 0;

  // Waveform-generator //(phase accumulator and waveform-selector)

  for (Channel = 0, Voice = 0; Channel < 21; Channel += 7, Voice++) {

    const VoiceRenderParameters &voice = parameters.voices[Voice];
    WF = voice.waveform;
    WavGenOut = PrevWavGenOut[Channel];
    TestBit = ((WF & TEST_BITVAL) != 0);
    PhaseAccuPtr = &(PhaseAccu[Channel]);

    PhaseAccuStep = voice.phaseAccuStep;
    if (TestBit || ((WF & SYNC_BITVAL) && SyncSourceMSBrise))
      *PhaseAccuPtr = 0;
    else { // stepping phase-accumulator (oscillator)
      *PhaseAccuPtr += PhaseAccuStep;
      if (*PhaseAccuPtr >= 0x10000000)
        *PhaseAccuPtr -= 0x10000000;
    }
    *PhaseAccuPtr &= 0xFFFFFFF;
    MSB = *PhaseAccuPtr & 0x8000000;
    SyncSourceMSBrise = (MSB > (PrevPhaseAccu[Channel] & 0x8000000)) ? 1 : 0;

    if (WF & NOISE_BITVAL) {    // noise waveform
      Tmp = NoiseLFSR[Channel]; // clock LFSR all time if clockrate exceeds
                                // observable at given samplerate (last term):
      if (((*PhaseAccuPtr & 0x1000000) !=
           (PrevPhaseAccu[Channel] & 0x1000000)) ||
          PhaseAccuStep >= 0x1000000) {
        Feedback = ((Tmp & 0x400000) ^ ((Tmp & 0x20000) << 5)) != 0;
        Tmp =
            ((Tmp << 1) | Feedback | TestBit) &
            0x7FFFFF; // TEST-bit turns all bits in noise LFSR to 1 (on real SID
                      // slowly, in approx. 8000 microseconds ~ 300 samples)
        NoiseLFSR[Channel] = Tmp;
      } // we simply zero output when other waveform is mixed with noise. On
        // real SID LFSR continuously gets filled by zero and locks up. ($C1
        // waveform with pw<8 can keep it for a while.)
      WavGenOut = (WF & 0x70)
                      ? 0
                      : ((Tmp & 0x100000) >> 5) | ((Tmp & 0x40000) >> 4) |
                            ((Tmp & 0x4000) >> 1) | ((Tmp & 0x800) << 1) |
                            ((Tmp & 0x200) << 2) | ((Tmp & 0x20) << 5) |
                            ((Tmp & 0x04) << 7) | ((Tmp & 0x01) << 8);
    }

    else if (WF & PULSE_BITVAL) { // simple pulse
      PW = voice.pulseWidth;      // PW=0000..FFF0 from SID-register
      Utmp = *PhaseAccuPtr >> 12;

      if ((WF & 0xF0) ==
          PULSE_BITVAL) { // simple pulse, most often used waveform, make it
                          // sound as clean as possible (by making it trapezoid)
        Steepness = voice.pulseSteepness;
        if (TestBit)
          WavGenOut = 0xFFFF;
        else if (Utmp < PW) {                // rising edge (interpolation)
          PulsePeak = voice.pulseRisingPeak; // very thin pulses don't make a
                                             // full swing between 0 and max but
                                             // make a little spike
          Tmp = PulsePeak -
                (PW - Utmp) * Steepness;      // draw the slope from the peak
          WavGenOut = (Tmp < 0) ? 0 : Tmp;    // but stop at 0-level
        } else {                              // falling edge (interpolation)
          PulsePeak = voice.pulseFallingPeak; // very thin pulses don't make a
                                              // full swing between 0 and max
                                              // but make a little spike
          Tmp = (0xFFFF - Utmp) * Steepness -
                PulsePeak;                       // draw the slope from the peak
          WavGenOut = (Tmp >= 0) ? 0xFFFF : Tmp; // but stop at max-level
        }
      }

      else { // combined pulse
        WavGenOut = (Utmp >= PW || TestBit) ? 0xFFFF : 0;
        if (WF & TRI_BITVAL) {
          if (WF & SAW_BITVAL) { // pulse+saw+triangle (waveform nearly
                                 // identical to tri+saw)
            if (WavGenOut)
              WavGenOut = combinedWF(PulseSawTriangle, Utmp, Channel,
                                     voice.combinedWaveformFilter);
          } else { // pulse+triangle
            Tmp = *PhaseAccuPtr ^ ((WF & RING_BITVAL) ? RingSourceMSB : 0);
            if (WavGenOut)
              WavGenOut = combinedWF(PulseTriangle, Tmp >> 12, Channel,
                                     voice.combinedWaveformFilter);
          }
        } else if (WF & SAW_BITVAL) { // pulse+saw
          if (WavGenOut)
            WavGenOut = combinedWF(PulseSawtooth, Utmp, Channel,
                                   voice.combinedWaveformFilter);
        }
      }
    }

    else if (WF & SAW_BITVAL) {        // sawtooth
      WavGenOut = *PhaseAccuPtr >> 12; // saw (this row would be enough for
                                       // simple but aliased-at-high-pitch saw)
      if (WF & TRI_BITVAL)
        WavGenOut = combinedWF(SawTriangle, WavGenOut, Channel,
                               voice.combinedWaveformFilter); // saw+triangle
      else { // simple cleaned (bandlimited) saw
        Steepness = voice.sawSteepness;
        WavGenOut += (WavGenOut * Steepness) >>
                     16; // 1st half (rising edge) of asymmetric triangle-like
                         // saw waveform
        if (WavGenOut > 0xFFFF)
          WavGenOut =
              0xFFFF -
              (((WavGenOut - 0x10000) << 16) /
               Steepness); // 2nd half (falling edge, reciprocal steepness)
      }
    }

    else if (WF & TRI_BITVAL) { // triangle (this waveform has no harsh edges,
                                // so it doesn't suffer from strong aliasing at
                                // high pitches)
      Tmp = *PhaseAccuPtr ^ (WF & RING_BITVAL ? RingSourceMSB : 0);
      WavGenOut = (Tmp ^ (Tmp & 0x8000000 ? 0xFFFFFFF : 0)) >> 11;
    }

    WavGenOut &= 0xFFFF;
    if (WF & 0xF0)
      PrevWavGenOut[Channel] =
          WavGenOut; // emulate waveform 00 floating wave-DAC (utilized by
                     // SounDemon digis)
    else
      WavGenOut =
          PrevWavGenOut[Channel]; //(on real SID waveform00 decays, we just
                                  // simply keep the value to avoid clicks)
    PrevPhaseAccu[Channel] = *PhaseAccuPtr;
    RingSourceMSB = MSB;

    // routing the channel signal to either the filter or the unfiltered master
    // output depending on filter-switch SID-registers
    Envelope = model_ == Model::MOS6581
                   ? ADSR_DAC_6581[EnvelopeCounter[Channel]]
                   : EnvelopeCounter[Channel];
    if (voice.route == VOICE_ROUTE_FILTER) {
      filterInputSample += (((int)WavGenOut - 0x8000) * Envelope) >> 8;
    } else if (voice.route == VOICE_ROUTE_OUTPUT) {
      nonFiltedSample += (((int)WavGenOut - 0x8000) * Envelope) >> 8;
    }
  }
  NonFiltedSample = nonFiltedSample;
  FilterInputSample = filterInputSample;
  // update readable SID1-registers (some SID tunes might use 3rd channel
  // ENV3/OSC3 value as control)
  //  SID->C64->IObankRD[SID->BaseAddress+0x1B] = WavGenOut>>8; //OSC3, ENV3
  //  (some players rely on it, unfortunately even for timing)
  //  SID->C64->IObankRD[SID->BaseAddress+0x1C] = SID->EnvelopeCounter[14];
  //  //Envelope
}
