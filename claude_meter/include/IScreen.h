#pragma once

#include <Adafruit_SH110X.h>

// One render() method — makes adding future screens (WiFi status, battery,
// latency) trivial without touching DisplayManager.
class IScreen {
 public:
  virtual ~IScreen() = default;
  virtual void render(Adafruit_SH1106G &display) = 0;
};
