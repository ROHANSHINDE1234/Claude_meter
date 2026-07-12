#include <WiFi.h>

#include "ClaudeSessionProvider.h"
#include "DisplayManager.h"
#include "TokenUsageManager.h"
#include "TokenUsageScreen.h"
#include "hw_config.h"
#include "secrets.h"

ClaudeSessionProvider usageProvider;
TokenUsageManager usageManager(usageProvider);
DisplayManager displayManager;
TokenUsageScreen usageScreen(usageManager);

bool lastButtonState = HIGH;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());

  // Reset timestamps from Anthropic are Unix time; the ESP32 has no RTC of
  // its own, so it needs NTP to know what "now" is before it can compute a
  // countdown (see TokenUsageScreen::formatCountdown).
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  displayManager.begin();
  displayManager.setScreen(&usageScreen);

  connectWiFi();
  usageManager.begin();
}

void loop() {
  unsigned long now = millis();

  bool buttonState = digitalRead(PIN_BUTTON);
  if (lastButtonState == HIGH && buttonState == LOW) {
    usageManager.forceRefresh();
  }
  lastButtonState = buttonState;

  usageManager.tick(now);
  displayManager.tick(now);
}
