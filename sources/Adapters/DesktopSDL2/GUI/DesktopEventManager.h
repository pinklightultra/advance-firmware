#ifndef _DESKTOP_EVENT_MANAGER_H_
#define _DESKTOP_EVENT_MANAGER_H_

#include "UIFramework/SimpleBaseClasses/EventManager.h"
#include "UIFramework/BasicDatas/GUIEvent.h"
#include <SDL2/SDL.h>

#define MAX_JOY_COUNT 4

class DesktopEventManager : public EventManager {
public:
  DesktopEventManager();
  virtual ~DesktopEventManager();

  virtual bool Init() override;
  virtual int MainLoop() override;
  virtual void PostQuitMessage() override;
  virtual int GetKeyCode(const char *name) override;

protected:
  GUIEventPadButtonType mapKey(SDL_Keycode sym);
  GUIEventPadButtonType mapJoyButton(Uint8 button);

private:
  static bool finished_;
  SDL_Joystick *joystick_[MAX_JOY_COUNT];
};

#endif // _DESKTOP_EVENT_MANAGER_H_
