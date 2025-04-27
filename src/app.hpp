#include "freertos/idf_additions.h"

// 32-bits milliseconds
typedef uint32_t millis32_t;

typedef struct MorseMessage {
  millis32_t interval;
  bool is_silent;
} SignalSilent;

void setup(void);

void button_setup(void);
void button_loop(QueueHandle_t *morseQueue);
void button_end(void);

void display_setup(void);
void display_loop(QueueHandle_t *morseQueue);
void display_end(void);
