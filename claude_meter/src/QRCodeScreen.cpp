#include "../include/QRCodeScreen.h"

#include "../include/hw_config.h"

QRCodeScreen *QRCodeScreen::activeInstance_ = nullptr;

void QRCodeScreen::begin(const String &url, const char *caption) {
  url_ = url;
  caption_ = caption;
  rendered_ = false;
}

void QRCodeScreen::render(Adafruit_SH1106G &display) {
  if (rendered_) {
    return;
  }
  rendered_ = true;

  activeInstance_ = this;
  activeDisplay_ = &display;

  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = qrDisplayCallback;
  // Plenty for any "http://<ipv4>/" URL (23 chars worst case fits version 2);
  // the callback shrinks module size if a larger version ever gets picked.
  cfg.max_qrcode_version = 4;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;

  if (esp_qrcode_generate(&cfg, url_.c_str()) != ESP_OK) {
    // Encoding failed — fall back to plain text rather than drawing nothing.
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Open in browser:");
    display.println(url_);
    display.display();
  }

  activeInstance_ = nullptr;
  activeDisplay_ = nullptr;
}

void QRCodeScreen::qrDisplayCallback(esp_qrcode_handle_t qrcode) {
  if (activeInstance_ == nullptr || activeInstance_->activeDisplay_ == nullptr) {
    return;
  }
  QRCodeScreen &self = *activeInstance_;
  Adafruit_SH1106G &display = *self.activeDisplay_;

  int size = esp_qrcode_get_size(qrcode);
  int moduleSize = 2;
  while (moduleSize > 1 && size * moduleSize > OLED_HEIGHT) {
    moduleSize--;
  }
  int qrPixelSize = size * moduleSize;
  int xOffset = (OLED_WIDTH - qrPixelSize) / 2;
  int yOffset = (OLED_HEIGHT - qrPixelSize) / 2;

  display.clearDisplay();

  if (self.caption_ != nullptr) {
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(self.caption_);
    yOffset = 12;
  }

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      if (esp_qrcode_get_module(qrcode, x, y)) {
        display.fillRect(xOffset + x * moduleSize, yOffset + y * moduleSize, moduleSize, moduleSize, SH110X_WHITE);
      }
    }
  }

  display.display();
}
