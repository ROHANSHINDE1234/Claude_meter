#pragma once

// Copy this file to secrets.h (same folder) and fill in real values.
// secrets.h is gitignored — never commit real credentials there.

#define WIFI_SSID "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// From ~/.claude/.credentials.json (Linux) or %USERPROFILE%\.claude\.credentials.json
// (Windows), under claudeAiOauth.accessToken. Short-lived — see Approach A in
// token-usage-feature-decision.md: this is only the FIRST value, used to seed
// flash storage (TokenStore) on first boot. After that, update via the local
// web UI (TokenWebServer) instead of reflashing — see README section 3.4.
#define CLAUDE_ACCESS_TOKEN "paste-access-token-here"

// Password gate for the local token-update web page (http://<esp32-ip>/).
// Not real security — plain HTTP on the local network only. Don't reuse a
// password you care about.
#define WEB_UI_PASSWORD "choose-a-password"
