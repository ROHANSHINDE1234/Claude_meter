#include "../include/TokenUsageManager.h"

#include <Arduino.h>

#include "../include/hw_config.h"

void TokenUsageManager::begin() {
  forceRefresh();
}

void TokenUsageManager::tick(unsigned long nowMs) {
  // forceRefresh() always stamps lastFetchMs_, success or failure, so this
  // paces retries at POLL_INTERVAL_MS even while every attempt is failing —
  // without that, a persistent failure (e.g. a bad model id) turns every
  // loop() iteration into an API call.
  if (nowMs - lastFetchMs_ >= POLL_INTERVAL_MS) {
    forceRefresh();
  }
}

void TokenUsageManager::forceRefresh() {
  UsageData fresh;
  if (provider_.fetch(fresh)) {
    data_ = fresh;
    everFetched_ = true;
  }
  lastFetchMs_ = millis();
}

bool TokenUsageManager::isStale(unsigned long nowMs) const {
  if (!everFetched_) {
    return true;
  }
  return (nowMs - lastFetchMs_) >= STALE_AFTER_MS;
}
