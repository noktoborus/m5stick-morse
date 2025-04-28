#include "app.hpp"
#include "morse/morse.hpp"
#include <M5Unified.h>

void button_setup(void) { M5_LOGI("Button started"); }

void button_end(void) { M5_LOGI("Button stopped"); }

void button_loop() { vTaskDelay(1); }
