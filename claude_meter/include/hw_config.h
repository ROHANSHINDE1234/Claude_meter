#pragma once

#include <stdint.h>

// I2C / OLED (same wiring as oled_test — see README section 1.2)
constexpr int PIN_SDA = 21;
constexpr int PIN_SCL = 22;
constexpr uint8_t OLED_I2C_ADDR = 0x3C;
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;

// Force-refresh button
constexpr int PIN_BUTTON = 27;

// Usage polling
constexpr unsigned long POLL_INTERVAL_MS = 5UL * 60UL * 1000UL;  // how often to poll Anthropic
constexpr unsigned long STALE_AFTER_MS = 15UL * 60UL * 1000UL;   // how long a reading is trusted before the screen flags it stale

// Display
constexpr unsigned long RENDER_INTERVAL_MS = 1000UL;  // redraw cadence (reset countdown ticks down even between polls)
