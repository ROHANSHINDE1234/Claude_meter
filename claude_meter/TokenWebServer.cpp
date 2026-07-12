#include "TokenWebServer.h"

#include "secrets.h"

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
      "<textarea name='token' rows='4' cols='50'></textarea><br><br>"
      "<input type='submit' value='Update'>"
      "</form></body></html>";
  server_.send(200, "text/html", page);
}

void TokenWebServer::handleUpdate() {
  if (server_.arg("password") != WEB_UI_PASSWORD) {
    server_.send(401, "text/plain", "Wrong password");
    return;
  }

  String newToken = server_.arg("token");
  newToken.trim();
  if (newToken.length() == 0) {
    server_.send(400, "text/plain", "Empty token");
    return;
  }

  tokenStore_.set(newToken);
  server_.send(200, "text/plain", "Token updated. It takes effect on the next poll or button press.");
}
