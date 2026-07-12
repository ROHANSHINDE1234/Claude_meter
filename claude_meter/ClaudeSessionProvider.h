#pragma once

#include "IUsageProvider.h"

// Reads live Claude session usage by piggybacking on a real, tiny inference
// call and reading the rate-limit numbers back out of the response headers —
// see token-usage-feature-decision.md section 3.2 for why (there is no
// dedicated usage endpoint that works reliably).
class ClaudeSessionProvider : public IUsageProvider {
 public:
  bool fetch(UsageData &out) override;
};
