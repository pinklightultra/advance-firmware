#ifndef _SDL_AUDIO_DRIVER_H_
#define _SDL_AUDIO_DRIVER_H_

#include <SDL2/SDL.h>
#include "Adapters/DesktopSDL2/Process/SDLProcess.h"
#include "Services/Audio/AudioDriver.h"

class SDLAudioDriver;

class SDLAudioDriverThread : public SysThread {
public:
  SDLAudioDriverThread(SDLAudioDriver *driver);
  virtual bool Execute() override;
  void Notify();
  virtual void RequestTermination() override;

private:
  SysSemaphore *semaphore_;
  SDLAudioDriver *driver_;
};

class SDLAudioDriver : public AudioDriver {
public:
  SDLAudioDriver(AudioSettings &settings);
  virtual ~SDLAudioDriver();

  virtual bool InitDriver() override;
  virtual bool StartDriver() override;
  virtual void StopDriver() override;
  virtual void CloseDriver() override;
  virtual double GetStreamTime() override;
  virtual int GetPlayedBufferPercentage() override;
  virtual bool Interlaced() override { return true; }

  void OnChunkDone(Uint8 *stream, int len);
  int GetSampleRate();
  void SetVolume(int volume) { volume_ = volume; }
  int GetVolume() const { return volume_; }

private:
  bool isPlaying_;
  SDLAudioDriverThread *thread_;
  char *unalignedMain_;
  char *mainBuffer_;
  char *miniBlank_;
  int bufferPos_;
  int bufferSize_;
  int fragSize_;
  int sampleRate_;
  int volume_;
  Uint32 startTime_;
};

#endif // _SDL_AUDIO_DRIVER_H_
