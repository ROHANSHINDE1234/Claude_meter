#include "../include/TokenUsageScreen.h"

#include <time.h>

namespace {

// Formats seconds-until-reset as "Hh MMm". NTP sync (configTime, done once in
// claude_meter.ino after WiFi connects) is what makes time(nullptr) meaningful
// here — the reset fields from Anthropic are Unix timestamps, and the ESP32
// has no RTC of its own to compare them against otherwise.
void formatCountdown(uint32_t resetUnix, char *buf, size_t bufLen) {
  time_t now = time(nullptr);
  if (resetUnix == 0 || now < 1700000000) {  // no data yet, or clock not synced
    snprintf(buf, bufLen, "--:--");
    return;
  }
  long remaining = static_cast<long>(resetUnix) - static_cast<long>(now);
  if (remaining < 0) remaining = 0;
  int hours = remaining / 3600;
  int minutes = (remaining % 3600) / 60;
  snprintf(buf, bufLen, "%dh %02dm", hours, minutes);
}

// Small pixel-art smiley — the font has no emoji glyphs, so this is hand-drawn
// with basic GFX primitives rather than text.
void drawSmiley(Adafruit_SH1106G &display, int cx, int cy, int r) {
  display.drawCircle(cx, cy, r, SH110X_WHITE);
  display.fillCircle(cx - 2, cy - 2, 1, SH110X_WHITE);  // left eye
  display.fillCircle(cx + 2, cy - 2, 1, SH110X_WHITE);  // right eye
  display.drawLine(cx - 3, cy + 2, cx, cy + 3, SH110X_WHITE);
  display.drawLine(cx, cy + 3, cx + 3, cy + 2, SH110X_WHITE);
}

}  // namespace

void TokenUsageScreen::render(Adafruit_SH1106G &display) {
  const UsageData &data = manager_.current();
  unsigned long now = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);

  if (!data.valid) {
    display.println("Claude usage");
    display.println("No data yet...");
    display.display();
    return;
  }

  char countdown[16];
  formatCountdown(data.fiveHourResetUnix, countdown, sizeof(countdown));

  // Layout uses the same 1px gap between every row (header/percentage/bar/
  // label/countdown), rather than letting whatever space text size changes
  // happened to leave behind (which read as inconsistent spacing).
  constexpr int kGap = 1;
  int y = 0;

  display.setTextSize(1);
  display.setCursor(0, y);
  display.print("Claude usage");  // h=8
  y += 8 + kGap;

  display.setTextSize(2);
  display.setCursor(0, y);  // h=16
  display.printf("%d%%", static_cast<int>(data.fiveHourUtilization * 100.0f + 0.5f));
  drawSmiley(display, 92, y + 8, 6);
  y += 16 + kGap;

  // Horizontal usage bar, like the one on claude.ai — outlined rectangle is
  // the 0-100% range, the filled portion is how much of it is used. 5px
  // (not 6) so the six rows plus five uniform gaps fit the 64px panel
  // without clipping the last (STALE) row.
  constexpr int kBarHeight = 5;
  display.drawRect(0, y, 128, kBarHeight, SH110X_WHITE);
  int innerWidth = 128 - 2;
  int filled = static_cast<int>(data.fiveHourUtilization * innerWidth + 0.5f);
  if (filled < 0) filled = 0;
  if (filled > innerWidth) filled = innerWidth;
  if (filled > 0) {
    display.fillRect(1, y + 1, filled, kBarHeight - 2, SH110X_WHITE);
  }
  y += kBarHeight + kGap;

  display.setTextSize(1);
  display.setCursor(0, y);
  display.print("Resets in:");  // h=8
  y += 8 + kGap;

  display.setTextSize(2);
  display.setCursor(0, y);  // h=16
  display.print(countdown);
  y += 16;  // no trailing gap — STALE (if shown) sits right after, still 63px total

  if (manager_.isStale(now)) {
    display.setTextSize(1);
    display.setCursor(0, y);
    display.print("STALE - press btn");
  }

  display.display();
}
