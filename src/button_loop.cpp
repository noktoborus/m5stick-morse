#include "app.hpp"
#include <M5Unified.h>

void button_setup(void) { M5_LOGI("Button started"); }

void button_end(void) { M5_LOGI("Button stopped"); }

millis32_t last_msec = 0;

void button_loop(QueueHandle_t *morseQueue) {
  M5.update();

  if (M5.BtnA.wasChangePressed()) {
    millis32_t msec = M5.getUpdateMsec();
    SignalSilent message = {
        .interval = (msec - last_msec),
        .is_silent = M5.BtnA.wasPressed(),
    };
    if (xQueueSend(*morseQueue, &message, 1) != pdPASS) {
      M5_LOGI("drop morse interval: no space in queue");
    }
    last_msec = msec;
  }

  if (M5.BtnPWR.wasPressed()) {
    esp_restart();
  }

  vTaskDelay(1);
}
