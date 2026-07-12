#pragma once

#include "UsageData.h"

class IUsageProvider {
 public:
  virtual ~IUsageProvider() = default;

  // Attempts a fresh reading. Returns true and fills out on success.
  virtual bool fetch(UsageData &out) = 0;
};
