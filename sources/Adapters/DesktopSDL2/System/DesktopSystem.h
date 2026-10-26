#ifndef _DESKTOP_SYSTEM_H_
#define _DESKTOP_SYSTEM_H_

#include "System/System/System.h"
#include "UIFramework/SimpleBaseClasses/EventManager.h"

class DesktopSystem : public System {
public:
  static void Boot(int argc, char **argv);
  static void Shutdown();
  static int MainLoop();

public: // System implementation
  virtual unsigned long GetClock() override;
  virtual void GetBatteryState(BatteryState &state) override;
  virtual void SetDisplayBrightness(unsigned char value) override;
  virtual void Sleep(int millisec);
  virtual void PostQuitMessage() override;
  virtual unsigned int GetMemoryUsage() override;
  virtual void PowerDown() override;
  virtual void SystemBootloader() override;
  virtual void SystemReboot() override;
  virtual void SystemPutChar(int c) override;
  virtual uint32_t GetRandomNumber() override;
  virtual uint32_t GetDeviceIdentitySeed() override;
  virtual uint32_t Micros() override;
  virtual uint32_t Millis() override;

private:
  static EventManager *eventManager_;
};

#endif // _DESKTOP_SYSTEM_H_
