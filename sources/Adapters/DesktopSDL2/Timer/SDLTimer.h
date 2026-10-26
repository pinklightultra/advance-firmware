#ifndef _SDL_TIMER_H_
#define _SDL_TIMER_H_

#include <SDL2/SDL.h>
#include "System/Timer/Timer.h"

class SDLTimer : public I_Timer {
public:
  SDLTimer();
  virtual ~SDLTimer();
  virtual void SetPeriod(float msec) override;
  virtual bool Start() override;
  virtual void Stop() override;
  virtual float GetPeriod() override;
  Uint32 OnTimerTick();

private:
  float period_;
  float offset_;
  SDL_TimerID timer_;
  long lastTick_;
  bool running_;
};

class SDLTimerService : public TimerService {
public:
  virtual I_Timer *CreateTimer() override;
  virtual void TriggerCallback(int msec, timerCallback cb) override;
};

#endif // _SDL_TIMER_H_
