#include "SDLProcess.h"
#include <SDL2/SDL_thread.h>

int _SDLStartThread(void *argp) {
  SysThread *play = (SysThread *)argp;
  play->startExecution();
  return 0;
}

bool SDLProcessFactory::BeginThread(SysThread &thread) {
  SDL_CreateThread(_SDLStartThread, "SysThread", &thread);
  return true;
}

SysSemaphore *SDLProcessFactory::CreateNewSemaphore(int initialcount, int maxcount) {
  return new SDLSysSemaphore(initialcount, maxcount);
}

SDLSysSemaphore::SDLSysSemaphore(int initialcount, int maxcount) {
  handle_ = SDL_CreateSemaphore(initialcount);
}

SDLSysSemaphore::~SDLSysSemaphore() {
  if (handle_) {
    SDL_DestroySemaphore(handle_);
    handle_ = nullptr;
  }
}

SysSemaphoreResult SDLSysSemaphore::Wait() {
  if (!handle_) {
    return SSR_INVALID;
  }
  return (SysSemaphoreResult)SDL_SemWait(handle_);
}

SysSemaphoreResult SDLSysSemaphore::TryWait() {
  if (!handle_) {
    return SSR_INVALID;
  }
  return (SysSemaphoreResult)SDL_SemTryWait(handle_);
}

SysSemaphoreResult SDLSysSemaphore::WaitTimeout(unsigned long timeout) {
  if (!handle_) {
    return SSR_INVALID;
  }
  return (SysSemaphoreResult)SDL_SemWaitTimeout(handle_, timeout);
}

SysSemaphoreResult SDLSysSemaphore::Post() {
  if (!handle_) {
    return SSR_INVALID;
  }
  return (SysSemaphoreResult)SDL_SemPost(handle_);
}
