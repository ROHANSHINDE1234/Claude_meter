#pragma once

#include <Adafruit_SH110X.h>
#include <Wire.h>
#include <qrcode.h>  // ESP-IDF's espressif__qrcode component, bundled with the ESP32 core

#include "IScreen.h"

// Owns the OLED object and whichever screen is currently active.
class DisplayManager {
 public:
  DisplayManager();

  void begin();
  void setScreen(IScreen *screen) { screen_ = screen; }
  void tick(unsigned long nowMs);

  // Draws two lines of status text directly (bypassing the active screen) —
  // used as a fallback if QR encoding fails.
  void showStatus(const String &line1, const String &line2);

  // Draws a QR code encoding `url` (bypassing the active screen), with an
  // optional one-line caption above it. Falls back to showStatus() if
  // encoding fails.
  void showQRCode(const String &url, const char *caption = nullptr);

 private:
  // esp_qrcode_generate()'s display_func is a plain C callback with no
  // context pointer, so it's a static member reading scratch state set
  // immediately beforehand in showQRCode() — safe because generation is
  // synchronous and this sketch is single-threaded.
  static void qrDisplayCallback(esp_qrcode_handle_t qrcode);

  Adafruit_SH1106G display_;
  IScreen *screen_ = nullptr;
  unsigned long lastRenderMs_ = 0;

  static DisplayManager *activeInstance_;
  const char *pendingCaption_ = nullptr;
};
