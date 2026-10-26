#ifndef _SDL_AUDIO_H_
#define _SDL_AUDIO_H_

#include "Services/Audio/Audio.h"

class SDLAudioDriver;

class SDLAudio : public Audio {
public:
  SDLAudio(AudioSettings &hints);
  virtual ~SDLAudio();
  virtual void Init() override;
  virtual void Close() override;
  virtual int GetMixerVolume() override;
  virtual void SetMixerVolume(int volume) override;
  virtual int GetSampleRate();

private:
  SDLAudioDriver *drv_;
  int volume_;
};

#endif // _SDL_AUDIO_H_
