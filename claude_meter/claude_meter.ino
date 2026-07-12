#include <WiFi.h>

#include "ClaudeSessionProvider.h"
#include "DisplayManager.h"
#include "TokenStore.h"
#include "TokenUsageManager.h"
#include "TokenUsageScreen.h"
#include "TokenWebServer.h"
#include "hw_config.h"
#include "secrets.h"

TokenStore tokenStore;
ClaudeSessionProvider usageProvider(tokenStore);
TokenUsageManager usageManager(usageProvider);
DisplayManager displayManager;
TokenUsageScreen usageScreen(usageManager);
TokenWebServer tokenWebServer(tokenStore);

bool lastButtonState = HIGH;

// True from boot until the first successful usage fetch. While true, the
// QR code for the token-update page stays on screen (full space, no timer)
// instead of the usage screen — there's nothing useful to show yet if the
// seeded token isn't valid, and this is exactly when scanning it matters.
bool waitingForFirstFetch = true;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  displayManager.begin();
  displayManager.setScreen(&usageScreen);

  // Seeds NVS from secrets.h on first boot only; afterwards the stored value
  // (possibly updated via the web UI) takes precedence.
  tokenStore.begin(CLAUDE_ACCESS_TOKEN);

  connectWiFi();
  tokenWebServer.begin();

  String tokenPageUrl = String("http://") + WiFi.localIP().toString() + "/";
  displayManager.showQRCode(tokenPageUrl, "Scan to update token");

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

  if (waitingForFirstFetch) {
    if (usageManager.current().valid) {
      waitingForFirstFetch = false;  // token works — hand the screen to the usage display
    }
  } else {
    displayManager.tick(now);
  }

  tokenWebServer.handleClient();
}
