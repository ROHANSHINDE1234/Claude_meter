#pragma once

#include "IScreen.h"
#include "TokenUsageManager.h"

class TokenUsageScreen : public IScreen {
 public:
  explicit TokenUsageScreen(TokenUsageManager &manager) : manager_(manager) {}

  void render(Adafruit_SH1106G &display) override;

 private:
  TokenUsageManager &manager_;
};
