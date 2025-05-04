#include "app.hpp"
#include "freertos/idf_additions.h"
#include "morse/morse.hpp"

extern "C" {
void displayTask(void *) {
  display_setup();
  for (;;) {
    display_loop();
  }
  display_end();
  vTaskDelete(NULL);
}

void app_main(void) {
  setup();
  xTaskCreatePinnedToCore(displayTask, "displayTask", 8192, NULL, 1, NULL, 0);
}
}
