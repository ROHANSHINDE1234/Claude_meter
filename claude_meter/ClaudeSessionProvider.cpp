#include "ClaudeSessionProvider.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

namespace {

constexpr const char *kEndpoint = "https://api.anthropic.com/v1/messages";
// Cheapest available model — this call exists only to get rate-limit headers
// back, its own response is discarded (max_tokens: 1).
constexpr const char *kModel = "claude-haiku-4-5";

const char *kHeaderKeys[] = {
    "anthropic-ratelimit-unified-5h-utilization",
    "anthropic-ratelimit-unified-5h-reset",
    "anthropic-ratelimit-unified-7d-utilization",
    "anthropic-ratelimit-unified-7d-reset",
};

float parseUtilization(const String &value) {
  return value.length() ? value.toFloat() : 0.0f;
}

uint32_t parseResetTimestamp(const String &value) {
  return value.length() ? static_cast<uint32_t>(value.toInt()) : 0;
}

}  // namespace

bool ClaudeSessionProvider::fetch(UsageData &out) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[usage] WiFi not connected, skipping fetch");
    return false;
  }

  WiFiClientSecure client;
  // Prototype only — skips certificate validation. See "Known risks" in
  // CLAUDE.md / README before treating this as production-safe.
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, kEndpoint)) {
    Serial.println("[usage] https.begin() failed");
    return false;
  }
  https.collectHeaders(kHeaderKeys, sizeof(kHeaderKeys) / sizeof(kHeaderKeys[0]));

  https.addHeader("content-type", "application/json");
  https.addHeader("anthropic-version", "2023-06-01");
  https.addHeader("anthropic-beta", "oauth-2025-04-20");
  https.addHeader("authorization", String("Bearer ") + CLAUDE_ACCESS_TOKEN);
  https.addHeader("User-Agent", "claude-cli/1.0 (external, cli)");

  String body =
      String("{\"model\":\"") + kModel +
      "\",\"max_tokens\":1,\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}";

  int status = https.POST(body);
  Serial.printf("[usage] POST status: %d\n", status);
  if (status <= 0) {
    Serial.printf("[usage] HTTPClient error: %s\n", https.errorToString(status).c_str());
    https.end();
    return false;
  }
  if (status != 200) {
    Serial.println("[usage] response body: " + https.getString());
  }

  String fiveHUtil = https.header("anthropic-ratelimit-unified-5h-utilization");
  String fiveHReset = https.header("anthropic-ratelimit-unified-5h-reset");
  String sevenDUtil = https.header("anthropic-ratelimit-unified-7d-utilization");
  String sevenDReset = https.header("anthropic-ratelimit-unified-7d-reset");
  Serial.println("[usage] 5h-utilization=" + fiveHUtil + " 5h-reset=" + fiveHReset +
                  " 7d-utilization=" + sevenDUtil + " 7d-reset=" + sevenDReset);

  out.fiveHourUtilization = parseUtilization(fiveHUtil);
  out.fiveHourResetUnix = parseResetTimestamp(fiveHReset);
  out.sevenDayUtilization = parseUtilization(sevenDUtil);
  out.sevenDayResetUnix = parseResetTimestamp(sevenDReset);
  out.valid = out.fiveHourResetUnix != 0;

  https.end();
  return out.valid;
}
