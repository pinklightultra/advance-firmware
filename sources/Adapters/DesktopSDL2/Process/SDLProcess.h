#ifndef _SDL_PROCESS_H_
#define _SDL_PROCESS_H_

#include "System/Process/Process.h"
#include <SDL2/SDL.h>

class SDLProcessFactory : public SysProcessFactory {
public:
  virtual bool BeginThread(SysThread &) override;
  virtual SysSemaphore *CreateNewSemaphore(int initialcount = 0, int maxcount = 0) override;
};

class SDLSysSemaphore : public SysSemaphore {
public:
  SDLSysSemaphore(int initialcount = 0, int maxcount = 0);
  virtual ~SDLSysSemaphore();
  virtual SysSemaphoreResult Wait() override;
  virtual SysSemaphoreResult TryWait() override;
  virtual SysSemaphoreResult WaitTimeout(unsigned long) override;
  virtual SysSemaphoreResult Post() override;

private:
  SDL_sem *handle_;
};

#endif // _SDL_PROCESS_H_
