#pragma once

// Copy this file to secrets.h (same folder) and fill in real values.
// secrets.h is gitignored — never commit real credentials there.

#define WIFI_SSID "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// From ~/.claude/.credentials.json (Linux) or %USERPROFILE%\.claude\.credentials.json
// (Windows), under claudeAiOauth.accessToken. Short-lived — see Approach A in
// token-usage-feature-decision.md: re-paste a fresh token here when requests
// start failing with 401, rather than having the device refresh it itself.
#define CLAUDE_ACCESS_TOKEN "paste-access-token-here"
