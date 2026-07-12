#pragma once

#include <Arduino.h>

// Persists the Claude OAuth access token in flash (NVS) so a fresh token can
// be pushed in over the local web UI (TokenWebServer) without a USB reflash.
// On first boot (empty NVS), seeds from the secrets.h value.
class TokenStore {
 public:
  void begin(const char *seedToken);
  String get() const { return token_; }
  void set(const String &newToken);

 private:
  String token_;
};
