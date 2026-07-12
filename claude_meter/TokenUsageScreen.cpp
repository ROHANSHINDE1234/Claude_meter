#include "TokenUsageScreen.h"

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

  display.println("Claude usage");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.printf("%d%%\n", static_cast<int>(data.fiveHourUtilization * 100.0f + 0.5f));

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("Resets in %s\n", countdown);

  if (manager_.isStale(now)) {
    display.setCursor(0, 56);
    display.print("STALE - press btn");
  }

  display.display();
}
