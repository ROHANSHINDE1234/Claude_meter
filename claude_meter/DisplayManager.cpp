#include "DisplayManager.h"

#include "hw_config.h"

DisplayManager *DisplayManager::activeInstance_ = nullptr;

DisplayManager::DisplayManager() : display_(OLED_WIDTH, OLED_HEIGHT, &Wire, -1) {}

void DisplayManager::begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  display_.begin(OLED_I2C_ADDR, true);
  display_.clearDisplay();
  display_.display();
}

void DisplayManager::tick(unsigned long nowMs) {
  if (!screen_) {
    return;
  }
  if (nowMs - lastRenderMs_ < RENDER_INTERVAL_MS) {
    return;
  }
  lastRenderMs_ = nowMs;
  screen_->render(display_);
}

void DisplayManager::showStatus(const String &line1, const String &line2) {
  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setTextColor(SH110X_WHITE);
  display_.setCursor(0, 0);
  display_.println(line1);
  display_.println(line2);
  display_.display();
}

void DisplayManager::showQRCode(const String &url, const char *caption) {
  activeInstance_ = this;
  pendingCaption_ = caption;

  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = qrDisplayCallback;
  // Plenty for any "http://<ipv4>/" URL (23 chars worst case fits version 2);
  // the callback shrinks module size if a larger version ever gets picked.
  cfg.max_qrcode_version = 4;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;

  if (esp_qrcode_generate(&cfg, url.c_str()) != ESP_OK) {
    // Encoding failed — fall back to plain text rather than drawing nothing.
    showStatus("Open in browser:", url);
  }

  activeInstance_ = nullptr;
}

void DisplayManager::qrDisplayCallback(esp_qrcode_handle_t qrcode) {
  if (activeInstance_ == nullptr) {
    return;
  }
  DisplayManager &self = *activeInstance_;

  int size = esp_qrcode_get_size(qrcode);
  int moduleSize = 2;
  while (moduleSize > 1 && size * moduleSize > OLED_HEIGHT) {
    moduleSize--;
  }
  int qrPixelSize = size * moduleSize;
  int xOffset = (OLED_WIDTH - qrPixelSize) / 2;
  int yOffset = (OLED_HEIGHT - qrPixelSize) / 2;

  self.display_.clearDisplay();

  if (self.pendingCaption_ != nullptr) {
    self.display_.setTextSize(1);
    self.display_.setTextColor(SH110X_WHITE);
    self.display_.setCursor(0, 0);
    self.display_.println(self.pendingCaption_);
    yOffset = 12;
  }

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      if (esp_qrcode_get_module(qrcode, x, y)) {
        self.display_.fillRect(xOffset + x * moduleSize, yOffset + y * moduleSize, moduleSize, moduleSize,
                                SH110X_WHITE);
      }
    }
  }

  self.display_.display();
}
