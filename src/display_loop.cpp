#include "app.hpp"
#include "freertos/idf_additions.h"
#include "lgfx/v1/misc/enum.hpp"
#include "morse/morse.hpp"
#include "utility/Log_Class.hpp"
#include <M5Unified.h>

M5Canvas morse_canvas;
M5Canvas letter_canvas;
M5Canvas history_canvas;
M5Canvas ticktack_canvas;

void ticktack_canvas_setup(void) {
  ticktack_canvas.createSprite(80, 4);
  ticktack_canvas.fillSprite(TFT_DARKGREEN);
}

void ticktack_canvas_tick(void) {
  static int blocks[] = {TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_BLUE, TFT_ORANGE};
  static unsigned current = 0;
  const auto blocks_count = sizeof(blocks) / sizeof(blocks[0]);

  auto width = ticktack_canvas.width();
  auto height = ticktack_canvas.height();
  auto block_size = width / 5;
  auto offset = block_size * current;

  if (current == blocks_count)
    ticktack_canvas.fillSprite(TFT_BLACK);

  ticktack_canvas.fillRect(offset, 0, block_size, height, blocks[current]);
  if (current > blocks_count)
    current = 0;
  else
    current++;
  ticktack_canvas.pushSprite(&M5.Display, 0, 156);
}

void history_canvas_setup(void) {
  history_canvas.createSprite(80, 80);
  history_canvas.fillSprite(TFT_BLACK);
  history_canvas.setTextSize(2);
  history_canvas.setTextColor(TFT_DARKGREEN);
  history_canvas.setTextScroll(true);
}

void history_canvas_push(const char letter) {
  history_canvas.printf("%c", letter);
  history_canvas.pushSprite(&M5.Display, 0, 76);
}

void morse_canvas_setup(void) {
  morse_canvas.createSprite(80, 16);
  morse_canvas.fillSprite(TFT_ORANGE);
  morse_canvas.setTextSize(2);
  morse_canvas.setTextColor(TFT_BLACK);
  morse_canvas.setTextDatum(middle_center);
};

void morse_canvas_draw(const char code[MORSE_SEQUENCE_MAX]) {
  auto x = morse_canvas.width() / 2;
  auto y = morse_canvas.height() / 2;

  morse_canvas.fillSprite(TFT_ORANGE);
  morse_canvas.drawString(code, x, y);
  morse_canvas.pushSprite(&M5.Display, 0, 0);
};

void letter_canvas_clear() { letter_canvas.fillSprite(TFT_BLACK); };

void letter_canvas_setup(void) {
  letter_canvas.createSprite(80, 60);
  letter_canvas.fillSprite(TFT_BLACK);
  letter_canvas.setTextSize(4);
  letter_canvas.setTextDatum(middle_center);
  letter_canvas_clear();
};

void letter_canvas_draw(const char letter, bool is_done, bool is_error) {
  char string[] = {letter, '\0'};
  auto x = letter_canvas.width() / 2;
  auto y = letter_canvas.height() / 2;

  if (!is_done) {
    if (is_error) {
      letter_canvas.fillSprite(TFT_BLACK);
      letter_canvas.setTextColor(TFT_DARKGREEN);
    } else {
      letter_canvas.fillSprite(TFT_BLACK);
      letter_canvas.setTextColor(TFT_GREEN);
    }
  } else {
    if (is_error) {
      letter_canvas.fillSprite(TFT_RED);
      letter_canvas.setTextColor(TFT_BLACK);
    } else {
      letter_canvas.fillSprite(TFT_GREEN);
      letter_canvas.setTextColor(TFT_BLACK);
    }
  }

  letter_canvas.drawString(string, x, y);
  letter_canvas.pushSprite(&M5.Display, 0, 16);
};

void display_setup(void) {
  M5.Display.setBrightness(80);
  morse_canvas_setup();
  letter_canvas_setup();
  history_canvas_setup();
  ticktack_canvas_setup();
  ticktack_canvas_tick();
  morse_canvas_draw("MORSE");
  M5_LOGI("Display start with size: %" PRIu32 "x%" PRIu32, M5.Display.height(),
          M5.Display.width());
}

void display_end(void) { M5_LOGI("Display stopped"); }

MorseTimings timing = MorseTimings(100);
MorseSequence seq = MorseSequence();

millis32_t last_msec = 0;
MorseSignal last_signal = NO_SIGNAL;

typedef enum pause_state_e {
  PAUSE_STATE_NONE = 0,
  PAUSE_STATE_BEGIN = 1,
  PAUSE_STATE_LETTER = 2,
  PAUSE_STATE_WORD = 3,
} pause_state_e;

pause_state_e pause_state = PAUSE_STATE_NONE;

void morse_code_process_and_draw(bool is_signal, millis32_t interval) {
  if (interval == 0) {
    last_signal = NO_SIGNAL;
    pause_state = PAUSE_STATE_NONE;
  }

  if (is_signal) {
    if (timing.is_dit(interval) && last_signal != DIT) {
      bool is_valid;

      last_signal = DIT;
      seq.signal_propose(DIT, interval);
      is_valid = seq.is_valid_sequence();

      morse_canvas_draw(seq.code);
      letter_canvas_draw(seq.letter(), false, !is_valid);
    } else if (timing.is_dah(interval) && last_signal != DAH) {
      bool is_valid;

      last_signal = DAH;
      seq.signal_propose(DAH, interval);
      is_valid = seq.is_valid_sequence();
      morse_canvas_draw(seq.code);
      letter_canvas_draw(seq.letter(), false, !is_valid);
    }
  } else {
    if (!seq.is_empty()) {
      if (timing.is_letter(interval) && pause_state < PAUSE_STATE_LETTER) {
        bool is_complete = seq.is_complete_sequence();

        pause_state = PAUSE_STATE_LETTER;
        if (is_complete)
          history_canvas_push(seq.letter());
        letter_canvas_draw(seq.letter(), true, !is_complete);
        seq.done();
      } else if (pause_state < PAUSE_STATE_BEGIN) {
        bool is_valid = seq.is_valid_sequence();

        pause_state = PAUSE_STATE_BEGIN;
        seq.signal_commit();
        if (!is_valid) {
          letter_canvas_draw(seq.letter(), true, true);
          seq.clear();
        }
      }
    } else if (timing.is_word(interval) && pause_state < PAUSE_STATE_WORD) {
      pause_state = PAUSE_STATE_WORD;
      history_canvas_push(' ');
      letter_canvas_draw(' ', false, false);
    }
  }
}

bool adjust_last_is_signal;
bool adjust_message_displayed = false;
M5Canvas adjust_canvas;

bool time_adjust_loop(bool is_signal, millis32_t interval) {
  const unsigned adjust_pattern_len = 4;
  static const MorseSignal adjust_pattern[] = {DIT, DIT, DIT, DIT, NO_SIGNAL};

  if (!adjust_message_displayed) {
    adjust_canvas.createSprite(80, 154);
    adjust_canvas.setTextScroll(true);
    adjust_canvas.setTextDatum(top_center);
    adjust_canvas.setTextSize(1.8f);
    adjust_canvas.println("Type");
    adjust_canvas.println(adjust_pattern);
    adjust_canvas.println("to");
    adjust_canvas.println("adjust");
    adjust_canvas.println("DIT time");
    adjust_canvas.pushSprite(&M5.Display, 0, 16);
    adjust_message_displayed = true;
  }

  if (adjust_last_is_signal != is_signal) {
    adjust_last_is_signal = is_signal;
    if (!is_signal) {
      seq.signal_commit();
      adjust_canvas.printf("%" PRIu32 " ms\n", seq.interval[seq.len - 1]);
      adjust_canvas.pushSprite(&M5.Display, 0, 16);
      if (seq.len == adjust_pattern_len) {
        timing.adjust(seq.len, seq.code, seq.interval);
        seq.clear();
        adjust_canvas.fillScreen(TFT_BLACK);
        adjust_canvas.setCursor(0, 0);
        adjust_canvas.printf("Average\n%" PRIu32 " ms", timing.dit_avg);
        adjust_canvas.pushSprite(&M5.Display, 0, 16);
        return true;
      }
    }
  } else {
    seq.signal_propose(DIT, interval);
  }
  return false;
}

bool is_adjust_mode = true;
millis32_t first_signal_at = 0;
unsigned ticked = 0;

void display_loop() {
  millis32_t interval;
  bool is_signal;

  M5.update();

  if (M5.BtnB.wasPressed()) {
    esp_restart();
  }

  is_signal = M5.BtnA.isPressed();
  if (M5.BtnA.wasChangePressed()) {
    last_msec = M5.getUpdateMsec();
    interval = 0;
    if (is_signal && (first_signal_at == 0 || first_signal_at > last_msec)) {
      first_signal_at = last_msec;
    }
  }

  if (!is_adjust_mode) {
    unsigned new_tick_value =
        (M5.getUpdateMsec() - first_signal_at) / timing.dit_avg;

    if (new_tick_value != ticked) {
      ticked = new_tick_value;
      M5.Log.printf("ticks=%u time=%" PRIu32 " dit_avg=%" PRIu32 "\n", ticked,
                    M5.getUpdateMsec(), timing.dit_avg);
      ticktack_canvas_tick();
    }
  }

  if (!M5.BtnA.wasChangePressed()) {
    interval = M5.getUpdateMsec() - last_msec;

    if (interval < timing.dit_avg / 4) {
      vTaskDelay(1);
      return;
    }
  }

  if (!is_adjust_mode) {
    morse_code_process_and_draw(is_signal, interval);
  } else {
    if (time_adjust_loop(is_signal, interval))
      is_adjust_mode = false;
  }

  vTaskDelay(1);
}
