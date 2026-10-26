#include "SDLTimer.h"
#include "System/Console/Trace.h"

Uint32 timer_callback(Uint32 interval, void *param) {
  SDLTimer *timer = (SDLTimer *)param;
  return timer->OnTimerTick();
}

SDLTimer::SDLTimer() {
  running_ = false;
  timer_ = 0;
  period_ = 0.0f;
  offset_ = 0.0f;
  lastTick_ = 0;
}

SDLTimer::~SDLTimer() {
  if (running_) {
    Stop();
  }
}

void SDLTimer::SetPeriod(float msec) { period_ = msec; }

float SDLTimer::GetPeriod() { return period_; }

bool SDLTimer::Start() {
  if (!running_) {
    int intPeriod = (int)period_;
    offset_ = period_ - intPeriod;
    timer_ = SDL_AddTimer(intPeriod, timer_callback, this);
    running_ = (timer_ != 0);
  }
  return running_;
}

void SDLTimer::Stop() {
  if (running_) {
    SDL_RemoveTimer(timer_);
    running_ = false;
    timer_ = 0;
  }
}

Uint32 SDLTimer::OnTimerTick() {
  SetChanged();
  NotifyObservers();
  int intPeriod = (int)period_;
  offset_ += (period_ - intPeriod);
  if (offset_ > 1.0f) {
    intPeriod++;
    offset_ -= 1.0f;
  }
  return intPeriod;
}

I_Timer *SDLTimerService::CreateTimer() { return new SDLTimer(); }

struct SDLCallbackData {
  timerCallback cb_;
};

Uint32 trigger_callback(Uint32 interval, void *param) {
  SDLCallbackData *data = (SDLCallbackData *)param;
  data->cb_();
  delete data;
  return 0;
}

void SDLTimerService::TriggerCallback(int msec, timerCallback cb) {
  SDLCallbackData *data = new SDLCallbackData;
  data->cb_ = cb;
  SDL_AddTimer(msec, trigger_callback, data);
}
