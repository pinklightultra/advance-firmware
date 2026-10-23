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

#ifndef _ADVANCE_SID_H_
#define _ADVANCE_SID_H_

#include "Application/Audio/AudioSample.h"
#include <stdint.h>

#define ATTENUATION 26
#define C64_PAL_CPUCLK 985248

class AdvanceSID {
public:
  enum class Model : uint8_t { MOS6581, MOS8580 };

  // Keep the existing one-argument constructor fixed at 8580 for callers that
  // do not select a model explicitly.
  explicit AdvanceSID(uint16_t samplerate);
  AdvanceSID(Model model, uint16_t samplerate);
  Model GetModel() const { return model_; }
  void SetModel(Model model);
  void Reset();
  // Advance envelopes by one output sample using the PAL SID clock.
  void EmulateADSRs();
  // Advance envelopes by an explicit number of SID clock cycles.
  void EmulateADSRs(uint32_t cycles);
  void ResetChannel(unsigned char channel);
  int EmulateWaves();

  // Renders exactly `size` packed mono samples.
  void Render(AudioSample *buffer, int size);
  bool AreVoicesIdle() const;

  // SID-chip data:
  unsigned char Register[29];
  unsigned short SampleClockRatio;

  // ADSR-related:
  unsigned char ADSRstate[15];
  unsigned short RateCounter[15];
  unsigned char EnvelopeCounter[15];
  unsigned char ExponentCounter[15];
  // Wave-related:
  int PhaseAccu[15];     // 28bit precision instead of 24bit
  int PrevPhaseAccu[15]; //(integerized ClockRatio fractionals, WebSID has
                         // similar solution)
  unsigned char SyncSourceMSBrise;
  unsigned int RingSourceMSB;
  unsigned int NoiseLFSR[15];
  unsigned int PrevWavGenOut[15];
  unsigned char PrevWavData[15];
  // Filter-related:
  int PrevLowPass;
  int PrevBandPass;
  // Output-stage:
  int NonFiltedSample;
  int FilterInputSample;

private:
  Model model_;
  // Integer and fractional PAL clock cycles carried between output samples.
  uint32_t adsrCyclesPerSample_;
  uint16_t sampleRate_;
  uint16_t adsrClockRemainderPerSample_;
  uint16_t adsrClockRemainder_;

  static constexpr uint8_t RENDER_CHUNK_SIZE = 8;

  enum VoiceRoute : uint8_t {
    VOICE_ROUTE_FILTER,
    VOICE_ROUTE_OUTPUT,
    VOICE_ROUTE_DISABLED
  };

  struct ADSRVoiceRenderParameters {
    uint16_t attackPeriod;
    uint16_t decayPeriod;
    uint16_t releasePeriod;
    uint8_t sustainLevel;
    uint8_t gate;
  };

  struct ADSRRenderParameters {
    ADSRVoiceRenderParameters voices[3];
  };

  struct VoiceRenderParameters {
    uint32_t phaseAccuStep;
    uint32_t pulseWidth;
    int32_t pulseSteepness;
    int32_t pulseRisingPeak;
    int32_t pulseFallingPeak;
    int32_t sawSteepness;
    uint16_t combinedWaveformFilter;
    uint8_t waveform;
    VoiceRoute route;
  };

  struct RenderParameters {
    ADSRRenderParameters adsr;
    VoiceRenderParameters voices[3];
    int32_t cutoff;
    uint16_t filterCutoff;
    uint16_t resonance;
    uint8_t filterSwitchReso;
    uint8_t volumeBand;
    uint8_t mainVolume;
  };

  struct RenderChunk {
    uint32_t phases[3][RENDER_CHUNK_SIZE];
    uint8_t ringSourceMSBs[3][RENDER_CHUNK_SIZE];
    uint8_t envelopes[3][RENDER_CHUNK_SIZE];
    int32_t nonFiltedSamples[RENDER_CHUNK_SIZE];
    int32_t filterInputSamples[RENDER_CHUNK_SIZE];
  };

  void PrecalculateADSRParameters(ADSRRenderParameters &parameters);
  void PrecalculateRenderParameters(RenderParameters &parameters);
  uint32_t NextADSRCycleCount();
  void AdvanceADSRs(uint32_t cycles, const ADSRRenderParameters &parameters);
  void AdvanceADSRVoice(uint32_t cycles, const ADSRVoiceRenderParameters &voice,
                        uint8_t &state, uint16_t &rateCounter,
                        uint8_t &envelope, uint8_t &exponentCounter);
  void EmulateADSRChunk(const ADSRRenderParameters &parameters,
                        RenderChunk &chunk, uint8_t sampleCount);
  void EmulateWaveGeneratorsChunk(const RenderParameters &parameters,
                                  RenderChunk &chunk, uint8_t sampleCount);
  void EmulateOutputChunk(const RenderParameters &parameters,
                          const RenderChunk &chunk, AudioSample *buffer,
                          uint8_t sampleCount);
  void EmulateWaveGenerators(const RenderParameters &parameters);
  int EmulateWaves(const RenderParameters &parameters);
  int EmulateOutputStage(const RenderParameters &parameters);
  unsigned short combinedWF(const unsigned char *WFarray, unsigned short oscval,
                            unsigned char Channel, unsigned short Filt);
  unsigned short combinedWFChunk(const unsigned char *WFarray,
                                 unsigned short oscval, unsigned short Filt,
                                 uint8_t &previousWaveData);
};

#endif
