#include "DesktopEventManager.h"
#include "DesktopGUIWindowImp.h"
#include "Adapters/DesktopSDL2/Bridge/DesktopBridge.h"
#include "Adapters/DesktopSDL2/Display/DesktopDisplay.h"
#include "Application/Application.h"
#include "System/Console/Trace.h"
#include "System/System/System.h"

bool DesktopEventManager::finished_ = false;

DesktopEventManager::DesktopEventManager() {
  for (int i = 0; i < MAX_JOY_COUNT; i++) {
    joystick_[i] = nullptr;
  }
}

DesktopEventManager::~DesktopEventManager() {
  for (int i = 0; i < MAX_JOY_COUNT; i++) {
    if (joystick_[i]) {
      SDL_JoystickClose(joystick_[i]);
      joystick_[i] = nullptr;
    }
  }
}

bool DesktopEventManager::Init() {
  EventManager::Init();

  int joyCount = SDL_NumJoysticks();
  joyCount = (joyCount > MAX_JOY_COUNT) ? MAX_JOY_COUNT : joyCount;

  for (int i = 0; i < joyCount; i++) {
    joystick_[i] = SDL_JoystickOpen(i);
    if (joystick_[i]) {
      Trace::Log("EVENT", "Opened joystick %d: %s", i, SDL_JoystickName(joystick_[i]));
    }
  }

  return true;
}

GUIEventPadButtonType DesktopEventManager::mapKey(SDL_Keycode sym) {
  switch (sym) {
  case SDLK_LEFT:
    return EPBT_LEFT;
  case SDLK_RIGHT:
    return EPBT_RIGHT;
  case SDLK_UP:
    return EPBT_UP;
  case SDLK_DOWN:
    return EPBT_DOWN;

  case SDLK_RETURN:
  case SDLK_RETURN2:
  case SDLK_KP_ENTER:
  case SDLK_z:
  case SDLK_c:
    return EPBT_A;

  case SDLK_ESCAPE:
  case SDLK_BACKSPACE:
  case SDLK_x:
  case SDLK_v:
    return EPBT_B;

  case SDLK_LSHIFT:
  case SDLK_LCTRL:
  case SDLK_TAB:
  case SDLK_q:
    return EPBT_L;

  case SDLK_RSHIFT:
  case SDLK_RCTRL:
  case SDLK_LALT:
  case SDLK_RALT:
  case SDLK_e:
    return EPBT_R;

  case SDLK_SPACE:
    return EPBT_START;

  case SDLK_DELETE:
  case SDLK_BACKQUOTE:
    return EPBT_SELECT;

  case SDLK_F12:
    return EPBT_POWER;

  default:
    return EPBT_INVALID;
  }
}

GUIEventPadButtonType DesktopEventManager::mapJoyButton(Uint8 button) {
  switch (button) {
  case 0: // A / Cross
    return EPBT_A;
  case 1: // B / Circle
    return EPBT_B;
  case 2: // X / Square
    return EPBT_SELECT;
  case 3: // Y / Triangle
    return EPBT_A;
  case 4: // L1 / LB
    return EPBT_L;
  case 5: // R1 / RB
    return EPBT_R;
  case 6: // Back / Select
    return EPBT_SELECT;
  case 7: // Start
    return EPBT_START;
  case 8: // Guide / Home
    return EPBT_POWER;
  default:
    return EPBT_INVALID;
  }
}

int DesktopEventManager::MainLoop() {
  GUIWindow *appWindow = Application::GetInstance()->GetWindow();
  const Uint32 frameInterval = 1000 / 60; // 60 FPS
  Uint32 nextTick = SDL_GetTicks() + frameInterval;

  while (!finished_) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      unsigned long now = System::GetInstance()->GetClock();

      switch (event.type) {
      case SDL_QUIT:
        finished_ = true;
        break;

      case SDL_KEYDOWN: {
        // Alt+Enter or F11 toggle fullscreen
        if (event.key.keysym.sym == SDLK_F11 ||
            ((event.key.keysym.sym == SDLK_RETURN) && (event.key.keysym.mod & KMOD_ALT))) {
          SDL_Window *w = SDL_GL_GetCurrentWindow();
          if (w) {
            Uint32 flags = SDL_GetWindowFlags(w);
            SDL_SetWindowFullscreen(w, (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
          }
          break;
        }

        GUIEventPadButtonType padBtn = mapKey(event.key.keysym.sym);
        if (padBtn != EPBT_INVALID && appWindow) {
          GUIEvent guiEv(padBtn, ET_PADBUTTONDOWN, now, 0, 0, 0);
          appWindow->DispatchEvent(guiEv);
        }
        break;
      }

      case SDL_KEYUP: {
        GUIEventPadButtonType padBtn = mapKey(event.key.keysym.sym);
        if (padBtn != EPBT_INVALID && appWindow) {
          GUIEvent guiEv(padBtn, ET_PADBUTTONUP, now, 0, 0, 0);
          appWindow->DispatchEvent(guiEv);
        }
        break;
      }

      case SDL_JOYBUTTONDOWN: {
        GUIEventPadButtonType padBtn = mapJoyButton(event.jbutton.button);
        if (padBtn != EPBT_INVALID && appWindow) {
          GUIEvent guiEv(padBtn, ET_PADBUTTONDOWN, now, 0, 0, 0);
          appWindow->DispatchEvent(guiEv);
        }
        break;
      }

      case SDL_JOYBUTTONUP: {
        GUIEventPadButtonType padBtn = mapJoyButton(event.jbutton.button);
        if (padBtn != EPBT_INVALID && appWindow) {
          GUIEvent guiEv(padBtn, ET_PADBUTTONUP, now, 0, 0, 0);
          appWindow->DispatchEvent(guiEv);
        }
        break;
      }

      case SDL_JOYHATMOTION: {
        Uint8 val = event.jhat.value;
        if (appWindow) {
          GUIEvent evUp(EPBT_UP, (val & SDL_HAT_UP) ? ET_PADBUTTONDOWN : ET_PADBUTTONUP, now, 0, 0, 0);
          appWindow->DispatchEvent(evUp);
          GUIEvent evDown(EPBT_DOWN, (val & SDL_HAT_DOWN) ? ET_PADBUTTONDOWN : ET_PADBUTTONUP, now, 0, 0, 0);
          appWindow->DispatchEvent(evDown);
          GUIEvent evLeft(EPBT_LEFT, (val & SDL_HAT_LEFT) ? ET_PADBUTTONDOWN : ET_PADBUTTONUP, now, 0, 0, 0);
          appWindow->DispatchEvent(evLeft);
          GUIEvent evRight(EPBT_RIGHT, (val & SDL_HAT_RIGHT) ? ET_PADBUTTONDOWN : ET_PADBUTTONUP, now, 0, 0, 0);
          appWindow->DispatchEvent(evRight);
        }
        break;
      }

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_EXPOSED ||
            event.window.event == SDL_WINDOWEVENT_RESIZED ||
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          if (appWindow) {
            appWindow->Invalidate();
          }
        }
        break;

      case SDL_USEREVENT:
        if (event.user.code == 'C') {
          DesktopBridge_ProcessButtonMask((uint16_t)(uintptr_t)event.user.data1);
        } else if (event.user.code == 'R') {
          display_draw_screen();
          display_is_dirty = true;
          if (appWindow) {
            appWindow->Invalidate();
          }
        }
        break;
      }
    }

    Uint32 currentTicks = SDL_GetTicks();
    if (currentTicks >= nextTick) {
      if (appWindow) {
        appWindow->ClockTick();
        appWindow->Update(false);
      }
      nextTick = currentTicks + frameInterval;
    }

    SDL_Delay(1);
  }

  return 0;
}

void DesktopEventManager::PostQuitMessage() {
  finished_ = true;
}

int DesktopEventManager::GetKeyCode(const char *name) {
  return SDL_GetScancodeFromName(name);
}
