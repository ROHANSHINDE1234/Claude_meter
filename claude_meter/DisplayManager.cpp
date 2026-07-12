#include "DisplayManager.h"

#include "hw_config.h"

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
