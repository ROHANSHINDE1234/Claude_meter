#pragma once

#include <stdint.h>

struct UsageData {
  float fiveHourUtilization = 0.0f;  // fraction 0.0-1.0
  uint32_t fiveHourResetUnix = 0;
  float sevenDayUtilization = 0.0f;
  uint32_t sevenDayResetUnix = 0;
  bool valid = false;
};
