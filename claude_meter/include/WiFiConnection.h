#pragma once

#include <Arduino.h>

// Blocking WiFi STA connect + NTP time sync. ClaudeSessionProvider owns the
// HTTPS call itself and just checks WiFi.status(); this class owns getting
// onto the network in the first place.
class WiFiConnection {
 public:
  void begin(const char *ssid, const char *password);
  String localIP() const;
};
