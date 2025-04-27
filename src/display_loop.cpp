#include "app.hpp"
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
  // TODO: нужно переделать очередь ожиданиz символовЖ вместо ожидания word и
  // спользовать время ожидания dit, после каждого таймаута тикать бар
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
  char string[] = {letter, '\0'};

  history_canvas.printf(string);
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

  letter_canvas.fillSprite(TFT_BLACK);

  if (!is_done) {
    if (is_error) {
      letter_canvas.setTextColor(TFT_DARKGREEN);
    } else {
      letter_canvas.setTextColor(TFT_GREEN);
    }
  } else {
    if (is_error)
      letter_canvas.setTextColor(TFT_RED);
    else {
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

bool adjusted = false;
bool adjusted_message_printed = false;
char adjust_pattern[5] = {DIT, DIT, DIT, DIT, DIT};

bool adjust_loop(QueueHandle_t *morseQueue) {
  SignalSilent message;

  if (xQueueReceive(*morseQueue, &message, timing.dit_dah_threshold) ==
      pdPASS) {
    if (message.is_silent) {
      return false;
    }

    if (seq.len < sizeof(adjust_pattern)) {
      seq.push(adjust_pattern[seq.len], &message);
    }

    if (seq.len == sizeof(adjust_pattern)) {
      timing.adjust(seq.len, seq.code, seq.interval);
      seq.clear();
      return true;
    }
  }
  return false;
}

void draw_adjust_message() {
  M5.Display.setTextSize(2);
  M5.Display.printf(" Type\r\n");
  M5.Display.printf("%-*s\r\n", sizeof(adjust_pattern), adjust_pattern);
  M5.Display.printf("  to \r\n");
  M5.Display.printf("adjust\r\n");
  M5.Display.printf("timing\r\n");
}

void display_loop(QueueHandle_t *morseQueue) {
  SignalSilent message;

  if (!adjusted) {
    if (!adjusted_message_printed) {
      M5_LOGI("wait sequence for timing adjust: %-*s", sizeof(adjust_pattern),
              adjust_pattern);
      draw_adjust_message();
      adjusted_message_printed = true;
    }
    if (adjust_loop(morseQueue)) {
      adjusted = true;
      M5_LOGI("timing adjust done: avg_dit is %" PRIu32
              "ms, dit_dah_threshold: %" PRIu32
              "ms, word_pause_threshold: %" PRIu32 "ms",
              timing.dit_avg, timing.dit_dah_threshold,
              timing.word_pause_threshold);
    }
    return;
  }

  if (xQueueReceive(*morseQueue, &message, timing.dit_dah_threshold) ==
      pdPASS) {
    if (message.is_silent && !seq.is_empty()) {
      if (timing.is_dah(message.interval)) {
        M5_LOGI("LETTER-PAUSE: %" PRIu32 " ms", message.interval, seq.code);
        if (seq.is_complete_sequence()) {
          letter_canvas_draw(seq.letter(), true, false);
          history_canvas_push(seq.letter());
        } else {
          letter_canvas_draw(seq.letter(), true, true);
        }
        seq.done();
      }

      if (timing.is_word(message.interval)) {
        M5_LOGI("WORD-PAUSE %" PRIu32 " ms", message.interval);
        history_canvas_push(' ');
      }
      return;
    } else if (!message.is_silent) {
      const char *title;

      if (timing.is_dah(message.interval)) {
        seq.push(DAH, &message);
        title = "DAH";
      } else if (timing.is_dit(message.interval)) {
        seq.push(DIT, &message);
        title = "DIT";
      } else {
        M5_LOGI("unrecognized signal timing: %" PRIu32 " ms", message.interval);
        return;
      }

      if (!seq.is_empty()) {
        if (seq.is_valid_sequence()) {
          M5_LOGI("%s: %" PRIu32 " ms ('%s': '%c')", title, message.interval,
                  seq.code, seq.letter());
          letter_canvas_draw(seq.letter(), false, !seq.is_complete_sequence());
        } else {
          M5_LOGI("clear unknown/invalid sequence: %s", seq.code);
          seq.clear();
          letter_canvas_clear();
        }
        morse_canvas_draw(seq.code);
      }
    }
  } else if (!seq.is_empty()) {
    M5_LOGI("done, signals num: %u ('%s': %c)", seq.len, seq.code,
            seq.letter());
    morse_canvas_draw(seq.code);
    if (seq.is_complete_sequence()) {
      letter_canvas_draw(seq.letter(), true, false);
      history_canvas_push(seq.letter());
    } else {
      letter_canvas_draw(seq.letter(), true, true);
    }
    seq.done();
    return;
  } else {
    return;
  }
}
