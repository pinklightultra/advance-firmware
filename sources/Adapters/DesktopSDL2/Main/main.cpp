#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "Adapters/DesktopSDL2/Bridge/DesktopBridge.h"
#include "Adapters/DesktopSDL2/System/DesktopSystem.h"
#include "Adapters/DesktopSDL2/Tests/SmokeTest.h"
#include "Application/Application.h"
#include "System/Console/Trace.h"
#include <cstring>

int main(int argc, char *argv[]) {
  bool runSmokeTest = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--smoke-test") == 0) {
      runSmokeTest = true;
    }
    if (strcmp(argv[i], "--electron-bridge") == 0) {
      DesktopBridge_Enable();
    }
  }

  if (DesktopBridge_IsEnabled()) {
    DesktopBridge_Init();
  }

  SDL_SetMainReady();
  Trace::RegisterEtlErrorHandler();

  DesktopSystem::Boot(argc, argv);

  GUICreateWindowParams params;
  params.title = "picoTracker Advance";

  Application::GetInstance()->Init(params);

  if (runSmokeTest) {
    int res = RunDesktopSmokeTests();
    DesktopSystem::Shutdown();
    if (DesktopBridge_IsEnabled()) {
      DesktopBridge_Shutdown();
    }
    return res;
  }

  DesktopSystem::MainLoop();

  DesktopSystem::Shutdown();

  if (DesktopBridge_IsEnabled()) {
    DesktopBridge_Shutdown();
  }

  return 0;
}
