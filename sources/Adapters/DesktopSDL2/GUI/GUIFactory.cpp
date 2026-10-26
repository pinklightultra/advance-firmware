#include "GUIFactory.h"
#include "DesktopGUIWindowImp.h"

GUIFactory::GUIFactory() {}

GUIFactory::~GUIFactory() {}

I_GUIWindowImp &GUIFactory::CreateWindowImp(GUICreateWindowParams &p) {
  return *(new DesktopGUIWindowImp(p));
}

EventManager *GUIFactory::GetEventManager() { return &eventManager_; }
