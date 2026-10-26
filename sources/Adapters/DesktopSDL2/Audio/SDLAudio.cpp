#include "SDLAudio.h"
#include "SDLAudioDriver.h"
#include "Services/Audio/AudioOutDriver.h"
#include "System/Console/Trace.h"

SDLAudio::SDLAudio(AudioSettings &hints) : Audio(hints) {
  drv_ = nullptr;
  volume_ = 100;
}

SDLAudio::~SDLAudio() {
  delete drv_;
}

void SDLAudio::Init() {
  AudioSettings settings;
  settings.audioAPI_ = GetAudioAPI();
  settings.bufferSize_ = GetAudioBufferSize();
  settings.preBufferCount_ = GetAudioPreBufferCount();

  drv_ = new SDLAudioDriver(settings);
  AudioOut *out = new AudioOutDriver(*drv_);
  AddOutput(*out);
}

void SDLAudio::Close() {
  for (AudioOut *out : outputs_) {
    if (out) {
      out->Close();
    }
  }
}

int SDLAudio::GetSampleRate() {
  if (!drv_) {
    return 44100;
  }
  return drv_->GetSampleRate();
}

int SDLAudio::GetMixerVolume() { return volume_; }

void SDLAudio::SetMixerVolume(int volume) {
  volume_ = volume;
  if (drv_) {
    drv_->SetVolume(volume);
  }
}
