#include "app.hpp"
#include "morse/morse.hpp"
#include "utility/Log_Class.hpp"
#include <M5Unified.h>

void display_setup(void) {
  M5.Display.setTextSize(1);
  M5.Display.setBrightness(80);

  M5_LOGI("Display start");
}

void display_end(void) { M5_LOGI("Display stopped"); }

MorseTimings timing = MorseTimings(100);
MorseSequence seq = MorseSequence();

bool adjusted = false;
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

void display_loop(QueueHandle_t *morseQueue) {
  SignalSilent message;

  if (!adjusted) {
    M5_LOGI("wait sequence for timing adjust: %-*s", sizeof(adjust_pattern),
            adjust_pattern);
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
        seq.done();
      }

      if (timing.is_word(message.interval)) {
        M5_LOGI("WORD-PAUSE %" PRIu32 " ms", message.interval);
        // TODO: word done
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
        } else {
          M5_LOGI("clear unknown/invalid sequence: %s", seq.code);
          seq.clear();
        }
      }
    }
  } else if (!seq.is_empty()) {
    M5_LOGI("done, signals num: %u ('%s': %c)", seq.len, seq.code,
            seq.letter());
    seq.done();
    return;
  } else {
    return;
  }
}
