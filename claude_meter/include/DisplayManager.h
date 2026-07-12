#pragma once

#include <Adafruit_SH110X.h>
#include <Wire.h>

#include "IScreen.h"

// Owns the OLED object and whichever screen is currently active.
class DisplayManager {
 public:
  DisplayManager();

  void begin();
  void setScreen(IScreen *screen);
  void tick(unsigned long nowMs);

 private:
  Adafruit_SH1106G display_;
  IScreen *screen_ = nullptr;
  unsigned long lastRenderMs_ = 0;
};
