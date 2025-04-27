#include "app.hpp"
#include "utility/Log_Class.hpp"
#include <M5Unified.h>
#include <cstring>

#define DIT '.'
#define DAH '_'

// Maximum length of sequence
#define MORSE_SEQUENCE_MAX 10

// > Medium gap (between words): seven time units long (formerly five)
// - https://en.wikipedia.org/wiki/Morse_code#Representation,_timing,_and_speeds
#define MORSE_WORD_MULTIPLIER 5

// > short gap (between letters): three time units long
// - https://en.wikipedia.org/wiki/Morse_code#Representation,_timing,_and_speeds
#define MORSE_LETTER_MUTLIPLIER 3

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct Morse {
  char alpha_num;
  char code[MORSE_SEQUENCE_MAX];
} Morse;

const Morse MorseABC[] = {
    {'A', {DIT, DAH}},
    {'B', {DAH, DIT, DIT, DIT}},
    {'C', {DAH, DIT, DAH, DIT}},
    {'D', {DAH, DIT, DIT}},
    {'E', {DIT}},
    {'F', {DIT, DIT, DAH, DIT}},
    {'G', {DAH, DAH, DIT}},
    {'H', {DIT, DIT, DIT, DIT}},
    {'I', {DIT, DIT}},
    {'J', {DIT, DAH, DAH, DAH}},
    {'K', {DAH, DIT, DAH}},
    {'L', {DIT, DAH, DIT, DIT}},
    {'M', {DAH, DAH}},
    {'N', {DAH, DIT}},
    {'O', {DAH, DAH, DAH}},
    {'P', {DIT, DAH, DAH, DIT}},
    {'Q', {DAH, DAH, DIT, DAH}},
    {'R', {DIT, DAH, DIT}},
    {'S', {DIT, DIT, DIT}},
    {'T', {DAH}},
    {'U', {DIT, DIT, DAH}},
    {'V', {DIT, DIT, DIT, DAH}},
    {'W', {DIT, DAH, DAH}},
    {'X', {DAH, DIT, DIT, DAH}},
    {'Y', {DAH, DIT, DAH, DAH}},
    {'Z', {DAH, DAH, DIT, DIT}},
    {'1', {DIT, DAH, DAH, DAH, DAH}},
    {'2', {DIT, DIT, DAH, DAH, DAH}},
    {'3', {DIT, DIT, DIT, DAH, DAH}},
    {'4', {DIT, DIT, DIT, DIT, DAH}},
    {'5', {DIT, DIT, DIT, DIT, DIT}},
    {'6', {DAH, DIT, DIT, DIT, DIT}},
    {'7', {DAH, DAH, DIT, DIT, DIT}},
    {'8', {DAH, DAH, DAH, DIT, DIT}},
    {'9', {DAH, DAH, DAH, DAH, DIT}},
    {'0', {DAH, DAH, DAH, DAH, DAH}},
};

// Maximum length of known sequences
// drop letter if exceeded
unsigned morse_sequence_threshold = 5;

unsigned len(const char *ptr) {
  unsigned i = 0;

  while (*(ptr + i) != '\0') {
    i++;
  }

  return i;
}

void display_setup(void) {
  M5.Display.setTextSize(1);
  M5.Display.setBrightness(80);
  M5.millis();

  const unsigned values = sizeof(MorseABC) / sizeof(MorseABC[0]);

  M5_LOGI("Process %u morse codes", values);
  for (unsigned i = 0; i < values; i++) {
    unsigned seq_len = len(MorseABC[i].code);

    M5_LOGI("%c: %u", MorseABC[i].alpha_num, seq_len);
    morse_sequence_threshold = MAX(seq_len, morse_sequence_threshold);
  }

  M5_LOGI("Max sequence: %u", morse_sequence_threshold);

  M5_LOGI("Display start");
}

void display_end(void) { M5_LOGI("Display stopped"); }

// To distinguish DIT from DAH '2' is enough
// because 'DAH is three time units long'
// This value can use to distinguish
// letter pause from dit/dah pause
#define DIT_DAH_THRESHOLD_MULTIPLIER 2

// > Medium gap (between words): seven time units long (formerly five)
// so, to distinguish DIT and DATH from '4' is enough
#define WORD_PAUSE_MULTIPLIER 4

class MorseTimings {
public:
  millis32_t dit_avg;
  millis32_t dit_dah_threshold;
  millis32_t word_pause_threshold;

  MorseTimings(millis32_t dit_avg) { this->set_dit_avg(dit_avg); }

  // True if `interval` is for dit
  bool is_dit(millis32_t interval) {
    return interval < this->dit_dah_threshold;
  }

  // True if `interval` is for dah
  bool is_dah(millis32_t interval) {
    return interval > this->dit_dah_threshold;
  }

  // True if interval is for word pause
  bool is_word(millis32_t interval) {
    return interval >= this->word_pause_threshold;
  }

  void set_dit_avg(millis32_t dit_avg) {
    this->dit_avg = dit_avg;
    this->dit_dah_threshold = dit_avg * DIT_DAH_THRESHOLD_MULTIPLIER;
    this->word_pause_threshold = dit_avg * WORD_PAUSE_MULTIPLIER;
  }

  // Adjust DIT time
  void adjust(unsigned length, char reference[MORSE_SEQUENCE_MAX],
              SignalSilent new_timings[MORSE_SEQUENCE_MAX]) {
    millis32_t sum = 0;
    unsigned c = 0;

    for (unsigned i = 0; i < length; i++) {
      if (reference[i] == DIT) {
        sum += new_timings[i].interval;
        c++;
      }
    }

    if (c) {
      this->set_dit_avg(sum / c);
    }
  }
};

class MorseSequence {
private:
  unsigned morse_sequence_threshold;
  char last_letter;

public:
  char code[MORSE_SEQUENCE_MAX];
  SignalSilent interval[MORSE_SEQUENCE_MAX];
  unsigned len;

  MorseSequence(unsigned morse_sequence_threshold) {
    this->morse_sequence_threshold = morse_sequence_threshold;
  }

  bool is_empty() { return this->len == 0; }

  bool is_full() { return this->len >= this->morse_sequence_threshold; }

  // True if current sequence exist in ABC
  bool is_valid_sequence() {
    const unsigned values = sizeof(MorseABC) / sizeof(MorseABC[0]);
    const Morse *better_match = NULL;

    if (!this->len)
      return false;

    for (unsigned i = 0; i < values; i++) {
      if (memcmp(this->code, MorseABC[i].code, this->len) == 0) {
        if (MorseABC[i].code[this->len] == '\0') {
          better_match = &MorseABC[i];
        } else if (!better_match) {
          better_match = &MorseABC[i];
        }
      }
    }

    if (better_match != NULL) {
      this->last_letter = better_match->alpha_num;
      return true;
    }

    return false;
  }

  // callable only after `::is_valid_sequence()`
  char letter() { return this->last_letter; }

  void push(char letter, SignalSilent *interval) {
    this->code[this->len] = letter;
    this->interval[this->len] = *interval;
    this->len++;
  }

  void done() { this->clear(); }

  void clear() {
    for (; this->len > 0; this->len--) {
      this->code[this->len] = '\0';
      this->interval[this->len] = SignalSilent{};
    }
  }
};

MorseTimings timing = MorseTimings(100);
MorseSequence seq = MorseSequence(morse_sequence_threshold);

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
