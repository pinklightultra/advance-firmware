#include "DesktopGUIWindowImp.h"
#include "Adapters/DesktopSDL2/Bridge/DesktopBridge.h"
#include "System/Console/Trace.h"
#include "Application/Model/Config.h"

#define to_rgb565(color) \
  ((uint16_t)(((color._r & 0b11111000) << 8) | ((color._g & 0b11111100) << 3) | (color._b >> 3)))

DesktopGUIWindowImp *DesktopGUIWindowImp::instance_ = nullptr;

DesktopGUIWindowImp::DesktopGUIWindowImp(GUICreateWindowParams &p) {
  instance_ = this;
  lastColorIdx_ = 1;

  window_ = SDL_CreateWindow(
      p.title ? p.title : "picoTracker Advance",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      720, 720,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!window_) {
    Trace::Error("DISPLAY", "Failed to create SDL window: %s", SDL_GetError());
    return;
  }

  SDL_Surface *icon = SDL_LoadBMP("lgpt_icon.bmp");
  if (icon) {
    SDL_SetWindowIcon(window_, icon);
    SDL_FreeSurface(icon);
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) {
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
  }

  if (renderer_) {
    SDL_RenderSetLogicalSize(renderer_, 720, 720);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR565,
                                  SDL_TEXTUREACCESS_STREAMING, 720, 720);
  }

  Config *config = Config::GetInstance();
  if (config) {
    auto uiFontVar = (WatchedVariable *)config->FindVariable(FourCC::VarUIFont);
    if (uiFontVar) {
      uiFontVar->AddObserver(*this);
      auto uifontIndex = uiFontVar->GetInt();
      display_set_font_index(uifontIndex);
    }
  }
}

DesktopGUIWindowImp::~DesktopGUIWindowImp() {
  if (texture_) {
    SDL_DestroyTexture(texture_);
    texture_ = nullptr;
  }
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  instance_ = nullptr;
}

color_t DesktopGUIWindowImp::GetColor(GUIColor &c) {
  display_set_palette_color(c._paletteIndex, to_rgb565(c));
  return (color_t)c._paletteIndex;
}

void DesktopGUIWindowImp::SetColor(GUIColor &c) {
  color_t color = GetColor(c);
  lastColorIdx_ = color;
  display_set_foreground(color);
}

void DesktopGUIWindowImp::DrawRect(GUIRect &r) {
  display_fill_rect(lastColorIdx_, r.Left(), r.Top(), r.Width(), r.Height());
}

void DesktopGUIWindowImp::DrawChar(const char c, const GUIPoint &pos,
                                   const GUITextProperties &p) {
  uint8_t x = pos._x / 8;
  uint8_t y = pos._y / 8;
  display_set_cursor(x, y);
  display_putc(c, p.invert_);
}

void DesktopGUIWindowImp::DrawString(const char *string, const GUIPoint &pos,
                                     const GUITextProperties &p, bool overlay) {
  display_set_cursor(pos._x, pos._y);
  display_print(string, p.invert_);
}

GUIRect DesktopGUIWindowImp::GetRect() {
  return GUIRect(0, 0, 320, 240);
}

void DesktopGUIWindowImp::Invalidate() {}

void DesktopGUIWindowImp::Flush() {
  display_draw_changed();
  if (renderer_ && texture_) {
    SDL_UpdateTexture(texture_, NULL, framebuffer, 720 * sizeof(uint16_t));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    SDL_RenderPresent(renderer_);
  }
  if (DesktopBridge_IsEnabled() && display_is_dirty) {
    DesktopBridge_SendFrame(framebuffer, 720, 720);
    display_is_dirty = false;
  }
}

void DesktopGUIWindowImp::Lock() {}
void DesktopGUIWindowImp::Unlock() {}

void DesktopGUIWindowImp::Clear(GUIColor &c, bool overlay) {
  color_t backgroundColor = GetColor(c);
  display_set_background(backgroundColor);
  display_clear(backgroundColor);
}

void DesktopGUIWindowImp::ClearTextRect(GUIRect &r) {}

void DesktopGUIWindowImp::PushEvent(GUIEvent &event) {}

void DesktopGUIWindowImp::Update(Observable &o, I_ObservableData *d) {
  WatchedVariable &v = (WatchedVariable &)o;
  if (v.GetID() == FourCC::VarUIFont) {
    display_set_font_index(v.GetInt());
  }
}
