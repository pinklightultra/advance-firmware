#ifndef _DESKTOP_GUI_FACTORY_H_
#define _DESKTOP_GUI_FACTORY_H_

#include "UIFramework/Interfaces/I_GUIWindowFactory.h"
#include "DesktopEventManager.h"

class GUIFactory : public I_GUIWindowFactory {
public:
  GUIFactory();
  virtual ~GUIFactory();

  virtual I_GUIWindowImp &CreateWindowImp(GUICreateWindowParams &) override;
  virtual EventManager *GetEventManager() override;

private:
  DesktopEventManager eventManager_;
};

#endif // _DESKTOP_GUI_FACTORY_H_
