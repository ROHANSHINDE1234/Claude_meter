#include "../include/TokenWebServer.h"

#include "../include/secrets.h"

void TokenWebServer::begin() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/update", HTTP_POST, [this]() { handleUpdate(); });
  server_.begin();
}

void TokenWebServer::handleRoot() {
  const char *page =
      "<html><body>"
      "<h3>Claude Meter &mdash; update access token</h3>"
      "<form method='POST' action='/update'>"
      "Password: <input type='password' name='password'><br><br>"
      "New token:<br>"
      "<textarea name='token' rows='4' cols='50' autocapitalize='off' autocorrect='off' "
      "spellcheck='false'></textarea><br><br>"
      "<input type='submit' value='Update'>"
      "</form></body></html>";
  server_.send(200, "text/html", page);
}

void TokenWebServer::handleUpdate() {
  if (server_.arg("password") != WEB_UI_PASSWORD) {
    server_.send(401, "text/plain", "Wrong password");
    return;
  }

  // Strip ALL whitespace, not just leading/trailing — a real access token
  // never legitimately contains any, but mobile copy-paste (PC -> notes app
  // -> phone -> this form) can silently introduce a stray internal space or
  // newline that breaks the Authorization header without looking wrong.
  String rawToken = server_.arg("token");
  String newToken;
  newToken.reserve(rawToken.length());
  for (size_t i = 0; i < rawToken.length(); i++) {
    char c = rawToken.charAt(i);
    if (!isspace(static_cast<unsigned char>(c))) {
      newToken += c;
    }
  }

  if (newToken.length() == 0) {
    server_.send(400, "text/plain", "Empty token");
    return;
  }

  tokenStore_.set(newToken);
  usageManager_.forceRefresh();  // verify immediately instead of waiting for the next scheduled poll

  if (usageManager_.current().valid) {
    server_.send(200, "text/plain", "Token updated and verified - usage data refreshed.");
  } else {
    server_.send(200, "text/plain",
                  "Token saved, but the verification fetch failed. Double-check the token value; "
                  "the device will keep retrying automatically.");
  }
}
