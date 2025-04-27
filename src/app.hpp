#include "freertos/idf_additions.h"

void setup(void);

void button_setup(void);
void button_loop(QueueHandle_t *morseQueue);
void button_end(void);

void display_setup(void);
void display_loop(QueueHandle_t *morseQueue);
void display_end(void);
