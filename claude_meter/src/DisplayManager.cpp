#include "../include/DisplayManager.h"

#include "../include/hw_config.h"

DisplayManager::DisplayManager() : display_(OLED_WIDTH, OLED_HEIGHT, &Wire, -1) {}

void DisplayManager::begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  display_.begin(OLED_I2C_ADDR, true);
  display_.clearDisplay();
  display_.display();
}

void DisplayManager::setScreen(IScreen *screen) {
  screen_ = screen;
  if (screen_) {
    // Render immediately rather than waiting for the next periodic tick, so
    // screen swaps (e.g. boot QR -> usage screen) appear instantly instead
    // of after up to RENDER_INTERVAL_MS of a blank/stale display.
    screen_->render(display_);
    lastRenderMs_ = millis();
  }
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
