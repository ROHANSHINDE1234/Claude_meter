#include "../include/WiFiConnection.h"

#include <WiFi.h>

void WiFiConnection::begin(const char *ssid, const char *password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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

String WiFiConnection::localIP() const {
  return WiFi.localIP().toString();
}
