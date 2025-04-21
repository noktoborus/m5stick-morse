#include "M5GFX.h"
#include <M5Unified.h>

void setup()
{
  auto cfg = M5.config();
  cfg.internal_mic = true;
  M5.begin(cfg);
  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_NONE);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, false);
  M5.Log.printf("M5.Log Morse start\n");

  int display_count = M5.getDisplayCount();

  for (int i = 0; i < display_count; ++i) {
    m5gfx::M5GFX display = M5.getDisplay(i);

    int textsize = 2;
    display.setTextSize(textsize);
    display.setBrightness(120);
    display.printf("No.%d\n", i);
  }

  M5_LOGI("setup end");
}

uint32_t last_msec = 0;

void loop()
{
  M5.update();
  m5gfx::M5GFX display = M5.getDisplay(0);

  if (M5.BtnA.wasChangePressed()) {
      uint32_t msec = M5.getUpdateMsec();
      display.printf("%" PRIu32 " \n", msec - last_msec);
      last_msec = msec;
  }


  if (M5.BtnA.wasPressed()) {
    M5_LOGI("Push BtnA");
  }
  if (M5.BtnPWR.wasClicked()) {
#ifdef ARDUINO
    esp_restart();
#endif
  }

  lgfx::v1::delay(1);
}

extern "C"
{

    void loopTask(void*)
    {
    setup();
    for (;;) {
        loop();
    }
    vTaskDelete(NULL);
    }

    void app_main(void)
    {
        xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
    }
}
