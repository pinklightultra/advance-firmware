#include "DesktopSystem.h"
#include "Adapters/DesktopSDL2/Audio/SDLAudio.h"
#include "Adapters/DesktopSDL2/FileSystem/DesktopFileSystem.h"
#include "Adapters/DesktopSDL2/GUI/GUIFactory.h"
#include "Adapters/DesktopSDL2/Midi/DummyMidi.h"
#include "Adapters/DesktopSDL2/Process/SDLProcess.h"
#include "Adapters/DesktopSDL2/System/DesktopSamplePool.h"
#include "Adapters/DesktopSDL2/Timer/SDLTimer.h"
#include "Adapters/DesktopSDL2/Platform/platform.h"
#include "Application/Model/Config.h"
#include "System/Console/Trace.h"
#include <SDL2/SDL.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>

EventManager *DesktopSystem::eventManager_ = nullptr;

void DesktopSystem::Boot(int argc, char **argv) {
  srand((unsigned int)time(nullptr));

  System::Install(new DesktopSystem());
  FileSystem::Install(new DesktopFileSystem());

  Config::GetInstance()->ProcessArguments(argc, argv);

  I_GUIWindowFactory::Install(new GUIFactory());
  TimerService::GetInstance()->Install(new SDLTimerService());
  SysProcessFactory::Install(new SDLProcessFactory());

  AudioSettings hint;
  hint.bufferSize_ = 1024;
  hint.preBufferCount_ = 8;
  Audio::Install(new SDLAudio(hint));

  MidiService::Install(new DummyMidi());
  SamplePool::Install(new DesktopSamplePool());

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0) {
    Trace::Error("SYSTEM", "SDL_Init failed: %s", SDL_GetError());
    return;
  }

  eventManager_ = I_GUIWindowFactory::GetInstance()->GetEventManager();
  if (eventManager_) {
    eventManager_->Init();
  }
}

void DesktopSystem::Shutdown() {
  if (Audio::GetInstance()) {
    delete Audio::GetInstance();
  }
  SDL_Quit();
}

int DesktopSystem::MainLoop() {
  if (eventManager_) {
    eventManager_->InstallMappings();
    return eventManager_->MainLoop();
  }
  return 0;
}

unsigned long DesktopSystem::GetClock() {
  return (unsigned long)SDL_GetTicks();
}

void DesktopSystem::GetBatteryState(BatteryState &state) {
  state.percentage = 100;
  state.voltage_mv = 4200;
  state.temperature_c = 25;
  state.charging = false;
  state.error = false;
}

void DesktopSystem::SetDisplayBrightness(unsigned char value) {}

void DesktopSystem::Sleep(int millisec) { SDL_Delay(millisec); }

void DesktopSystem::PostQuitMessage() {
  if (eventManager_) {
    eventManager_->PostQuitMessage();
  }
}

unsigned int DesktopSystem::GetMemoryUsage() { return 0; }

void DesktopSystem::PowerDown() {
  PostQuitMessage();
}

void DesktopSystem::SystemBootloader() {
  PostQuitMessage();
}

void DesktopSystem::SystemReboot() {
  PostQuitMessage();
}

void DesktopSystem::SystemPutChar(int c) {
  putchar(c);
}

uint32_t DesktopSystem::GetRandomNumber() {
  return (uint32_t)rand();
}

uint32_t DesktopSystem::GetDeviceIdentitySeed() {
  return 0x12345678;
}

uint32_t DesktopSystem::Micros() {
  return micros();
}

uint32_t DesktopSystem::Millis() {
  return millis();
}
