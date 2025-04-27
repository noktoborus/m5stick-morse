#include "app.hpp"
#include <M5Unified.h>

void setup(void) {
  auto cfg = M5.config();
  cfg.internal_mic = true;

  M5.begin(cfg);
  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_NONE);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, false);
  M5.Log.printf("M5.Log check\n");
}
