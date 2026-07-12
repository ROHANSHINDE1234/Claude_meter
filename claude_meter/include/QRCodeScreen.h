#pragma once

#include <Arduino.h>
#include <qrcode.h>  // ESP-IDF's espressif__qrcode component, bundled with the ESP32 core

#include "IScreen.h"

// Draws a QR code encoding a URL, with an optional one-line caption above
// it. Falls back to plain text if encoding fails.
class QRCodeScreen : public IScreen {
 public:
  void begin(const String &url, const char *caption = nullptr);

  void render(Adafruit_SH1106G &display) override;

 private:
  // esp_qrcode_generate()'s display_func is a plain C callback with no
  // context pointer, so it's a static member reading scratch state set
  // immediately beforehand in render() — safe because generation is
  // synchronous and this sketch is single-threaded.
  static void qrDisplayCallback(esp_qrcode_handle_t qrcode);

  String url_;
  const char *caption_ = nullptr;
  bool rendered_ = false;  // the image is static — draw it once, not every tick

  static QRCodeScreen *activeInstance_;
  Adafruit_SH1106G *activeDisplay_ = nullptr;
};
