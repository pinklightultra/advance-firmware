#include "DesktopBridge.h"
#include "Adapters/DesktopSDL2/Display/DesktopDisplay.h"
#include "Adapters/DesktopSDL2/GUI/DesktopGUIWindowImp.h"
#include "Application/Application.h"
#include "System/System/System.h"
#include "UIFramework/BasicDatas/GUIEvent.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

static bool s_enabled = false;
static FILE *s_bridgeOut = nullptr;
static SDL_Thread *s_inputThread = nullptr;
static uint16_t s_currentMask = 0;
static uint32_t s_rgbaBuf[DISPLAY_WIDTH * DISPLAY_HEIGHT];

static const GUIEventPadButtonType s_maskToButton[11] = {
    EPBT_LEFT,   // Bit 0: LEFT
    EPBT_DOWN,   // Bit 1: DOWN
    EPBT_RIGHT,  // Bit 2: RIGHT
    EPBT_UP,     // Bit 3: UP
    EPBT_L,      // Bit 4: L (NAV)
    EPBT_B,      // Bit 5: B (EDIT)
    EPBT_A,      // Bit 6: A (ENTER)
    EPBT_R,      // Bit 7: R (OPTION)
    EPBT_START,  // Bit 8: START (PLAY)
    EPBT_SELECT, // Bit 9: SELECT (VIEW)
    EPBT_POWER   // Bit 10: POWER
};

bool DesktopBridge_IsEnabled(void) {
  return s_enabled;
}

void DesktopBridge_Enable(void) {
  s_enabled = true;
}

static int SDLCALL bridge_input_thread(void *data) {
  (void)data;
  while (s_enabled) {
    int c = fgetc(stdin);
    if (c == EOF) {
      SDL_Event ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = SDL_QUIT;
      SDL_PushEvent(&ev);
      break;
    }
    if (c == 'Q') {
      SDL_Event ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = SDL_QUIT;
      SDL_PushEvent(&ev);
      break;
    } else if (c == 'C') {
      uint8_t bytes[2];
      if (fread(bytes, 1, 2, stdin) == 2) {
        uint16_t mask = (uint16_t)(bytes[0] | (bytes[1] << 8));
        SDL_Event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = SDL_USEREVENT;
        ev.user.code = 'C';
        ev.user.data1 = (void *)(uintptr_t)mask;
        SDL_PushEvent(&ev);
      }
    } else if (c == 'S') {
      uint8_t buf[9];
      if (fread(buf, 1, 9, stdin) == 9) {
        uint8_t down = buf[0];
        uint16_t scancode = (uint16_t)(buf[1] | (buf[2] << 8));
        uint32_t keycode = (uint32_t)(buf[3] | (buf[4] << 8) | (buf[5] << 16) | (buf[6] << 24));
        uint16_t mod = (uint16_t)(buf[7] | (buf[8] << 8));
        SDL_Event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
        ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
        ev.key.keysym.scancode = (SDL_Scancode)scancode;
        ev.key.keysym.sym = (SDL_Keycode)keycode;
        ev.key.keysym.mod = mod;
        SDL_PushEvent(&ev);
      }
    } else if (c == 'R') {
      SDL_Event ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = SDL_USEREVENT;
      ev.user.code = 'R';
      SDL_PushEvent(&ev);
    }
  }
  return 0;
}

void DesktopBridge_Init(void) {
  if (!s_enabled) return;

#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
  int bridge_fd = _dup(_fileno(stdout));
  if (bridge_fd >= 0) {
    s_bridgeOut = _fdopen(bridge_fd, "wb");
  }
  _dup2(_fileno(stderr), _fileno(stdout));
#else
  int bridge_fd = dup(fileno(stdout));
  if (bridge_fd >= 0) {
    s_bridgeOut = fdopen(bridge_fd, "wb");
  }
  dup2(fileno(stderr), fileno(stdout));
#endif

  SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

  s_inputThread = SDL_CreateThread(bridge_input_thread, "pt-bridge-in", nullptr);
  if (s_inputThread) {
    SDL_DetachThread(s_inputThread);
  }
}

void DesktopBridge_SendFrame(const uint16_t *fb, int width, int height) {
  if (!s_enabled || !s_bridgeOut || !fb) return;
  int count = width * height;
  for (int i = 0; i < count; ++i) {
    uint16_t p = fb[i];
    uint32_t r = (p & 0x1F) << 3;
    uint32_t g = ((p >> 5) & 0x3F) << 2;
    uint32_t b = ((p >> 11) & 0x1F) << 3;
    s_rgbaBuf[i] = 0xFF000000 | (b << 16) | (g << 8) | r;
  }
  uint8_t header[8] = {
    'P', 'T', 'F', 'R',
    (uint8_t)(width & 0xFF),
    (uint8_t)((width >> 8) & 0xFF),
    (uint8_t)(height & 0xFF),
    (uint8_t)((height >> 8) & 0xFF)
  };
  fwrite(header, 1, 8, s_bridgeOut);
  fwrite(s_rgbaBuf, 1, count * 4, s_bridgeOut);
  fflush(s_bridgeOut);
}

void DesktopBridge_ProcessButtonMask(uint16_t newMask) {
  GUIWindow *appWindow = Application::GetInstance()->GetWindow();
  if (!appWindow) return;
  unsigned long now = System::GetInstance()->GetClock();

  uint16_t diff = newMask ^ s_currentMask;
  for (int i = 0; i < 11; i++) {
    if (diff & (1 << i)) {
      bool isDown = (newMask & (1 << i)) != 0;
      GUIEvent guiEv(s_maskToButton[i], isDown ? ET_PADBUTTONDOWN : ET_PADBUTTONUP, now, 0, 0, 0);
      appWindow->DispatchEvent(guiEv);
    }
  }
  s_currentMask = newMask;
}

void DesktopBridge_Shutdown(void) {
  s_enabled = false;
  if (s_bridgeOut) {
    fflush(s_bridgeOut);
    fclose(s_bridgeOut);
    s_bridgeOut = nullptr;
  }
}
