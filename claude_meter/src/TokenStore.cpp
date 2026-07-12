#include "../include/TokenStore.h"

#include <Preferences.h>

namespace {
constexpr const char *kNamespace = "claude_meter";
constexpr const char *kKey = "access_token";
}  // namespace

void TokenStore::begin(const char *seedToken) {
  Preferences prefs;
  prefs.begin(kNamespace, /*readOnly=*/false);
  token_ = prefs.getString(kKey, "");
  if (token_.length() == 0 && seedToken != nullptr) {
    token_ = seedToken;
    prefs.putString(kKey, token_);
  }
  prefs.end();
}

void TokenStore::set(const String &newToken) {
  token_ = newToken;
  Preferences prefs;
  prefs.begin(kNamespace, /*readOnly=*/false);
  prefs.putString(kKey, token_);
  prefs.end();
}
