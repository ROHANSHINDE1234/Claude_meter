#pragma once

#include "IUsageProvider.h"
#include "UsageData.h"

// Owns the current usage reading and staleness tracking — single source of
// truth for "what does the display currently show."
class TokenUsageManager {
 public:
  explicit TokenUsageManager(IUsageProvider &provider) : provider_(provider) {}

  void begin();
  void tick(unsigned long nowMs);
  void forceRefresh();

  const UsageData &current() const { return data_; }
  bool isStale(unsigned long nowMs) const;

 private:
  IUsageProvider &provider_;
  UsageData data_;
  unsigned long lastFetchMs_ = 0;
  bool everFetched_ = false;
};
