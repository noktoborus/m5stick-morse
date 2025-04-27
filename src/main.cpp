#include "app.hpp"
#include "morse/morse.hpp"

QueueHandle_t morseQueue;

extern "C" {
void buttonTask(void *) {
  button_setup();
  for (;;) {
    button_loop(&morseQueue);
  }
  button_end();
  vTaskDelete(NULL);
}

void displayTask(void *) {
  display_setup();
  for (;;) {
    display_loop(&morseQueue);
  }
  display_end();
  vTaskDelete(NULL);
}

void app_main(void) {
  morseQueue = xQueueCreate(128, sizeof(SignalSilent));

  setup();
  xTaskCreatePinnedToCore(buttonTask, "buttonTask", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(displayTask, "displayTask", 8192, NULL, 1, NULL, 0);
}
}
