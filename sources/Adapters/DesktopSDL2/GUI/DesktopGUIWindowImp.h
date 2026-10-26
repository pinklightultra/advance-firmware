#ifndef _DESKTOP_GUI_WINDOW_IMP_H_
#define _DESKTOP_GUI_WINDOW_IMP_H_

#include "UIFramework/Interfaces/I_GUIWindowImp.h"
#include "Foundation/Observable.h"
#include "Adapters/DesktopSDL2/Display/DesktopDisplay.h"
#include <SDL2/SDL.h>

class DesktopGUIWindowImp : public I_GUIWindowImp, public I_Observer {
public:
  DesktopGUIWindowImp(GUICreateWindowParams &p);
  virtual ~DesktopGUIWindowImp();

  virtual void SetColor(GUIColor &) override;
  virtual void DrawRect(GUIRect &) override;
  virtual void DrawChar(const char c, const GUIPoint &pos,
                        const GUITextProperties &) override;
  virtual void DrawString(const char *string, const GUIPoint &pos,
                          const GUITextProperties &, bool overlay = false) override;
  virtual GUIRect GetRect() override;
  virtual void Invalidate() override;
  virtual void Flush() override;
  virtual void Lock() override;
  virtual void Unlock() override;
  virtual void Clear(GUIColor &, bool overlay = false) override;
  virtual void ClearTextRect(GUIRect &) override;
  virtual void PushEvent(GUIEvent &event) override;

  static DesktopGUIWindowImp *GetInstance() { return instance_; }

protected:
  static color_t GetColor(GUIColor &c);
  virtual void Update(Observable &o, I_ObservableData *d) override;

private:
  static DesktopGUIWindowImp *instance_;
  SDL_Window *window_;
  SDL_Renderer *renderer_;
  SDL_Texture *texture_;
  uint8_t lastColorIdx_;
};

#endif // _DESKTOP_GUI_WINDOW_IMP_H_
