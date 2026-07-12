// Headers live in include/, implementation (.cpp) in src/ — the one
// subfolder name the Arduino build system auto-compiles. Bare includes only
// resolve within the same folder, so this .ino needs the "include/" prefix,
// and each .cpp in src/ reaches back with "../include/...".
#include "include/ClaudeSessionProvider.h"
#include "include/DisplayManager.h"
#include "include/QRCodeScreen.h"
#include "include/TokenStore.h"
#include "include/TokenUsageManager.h"
#include "include/TokenUsageScreen.h"
#include "include/TokenWebServer.h"
#include "include/WiFiConnection.h"
#include "include/hw_config.h"
#include "include/secrets.h"

TokenStore tokenStore;
ClaudeSessionProvider usageProvider(tokenStore);
TokenUsageManager usageManager(usageProvider);
DisplayManager displayManager;
TokenUsageScreen usageScreen(usageManager);
QRCodeScreen qrScreen;
TokenWebServer tokenWebServer(tokenStore, usageManager);
WiFiConnection wifiConnection;

bool lastButtonState = HIGH;

// True from boot until the first successful usage fetch. While true, the
// QR code for the token-update page stays on screen — there's nothing
// useful to show yet if the stored token isn't valid, and this is exactly
// when scanning it matters.
bool waitingForFirstFetch = true;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  displayManager.begin();

  // Seeds NVS from secrets.h on first boot only; afterwards the stored value
  // (possibly updated via the web UI) takes precedence.
  tokenStore.begin(CLAUDE_ACCESS_TOKEN);

  wifiConnection.begin(WIFI_SSID, WIFI_PASSWORD);
  tokenWebServer.begin();

  qrScreen.begin(String("http://") + wifiConnection.localIP() + "/", "Scan to update token");
  displayManager.setScreen(&qrScreen);

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

  if (waitingForFirstFetch && usageManager.current().valid) {
    waitingForFirstFetch = false;  // token works — hand the screen to the usage display
    displayManager.setScreen(&usageScreen);
  }

  displayManager.tick(now);
  tokenWebServer.handleClient();
}
