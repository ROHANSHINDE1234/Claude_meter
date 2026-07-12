#pragma once

#include <WebServer.h>

#include "TokenStore.h"
#include "TokenUsageManager.h"

// Serves a small password-protected page for updating the Claude access
// token over the local network, without a USB reflash. Reachable only by
// devices on the same WiFi/hotspot as the ESP32 — this is a shared-password
// deterrent for a home/hotspot-only device, not real security (plain HTTP,
// no TLS). Don't reuse WEB_UI_PASSWORD anywhere sensitive.
class TokenWebServer {
 public:
  TokenWebServer(TokenStore &tokenStore, TokenUsageManager &usageManager)
      : tokenStore_(tokenStore), usageManager_(usageManager) {}

  void begin();
  void handleClient() { server_.handleClient(); }

 private:
  void handleRoot();
  void handleUpdate();

  WebServer server_{80};
  TokenStore &tokenStore_;
  TokenUsageManager &usageManager_;
};
