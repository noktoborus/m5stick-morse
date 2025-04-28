
#include "morse.hpp"

MorseTimings::MorseTimings(millis32_t dit_avg) { this->set_dit_avg(dit_avg); }

bool MorseTimings::is_dit(millis32_t interval) {
  return interval < this->dit_dah_threshold;
}

bool MorseTimings::is_dah(millis32_t interval) {
  return interval > this->dit_dah_threshold;
}

bool MorseTimings::is_letter(millis32_t interval) {
  return interval >= this->dit_dah_threshold;
}

bool MorseTimings::is_word(millis32_t interval) {
  return interval >= this->word_pause_threshold;
}

void MorseTimings::set_dit_avg(millis32_t dit_avg) {
  this->dit_avg = dit_avg;
  this->dit_dah_threshold = dit_avg * DIT_DAH_THRESHOLD_MULTIPLIER;
  this->word_pause_threshold = dit_avg * WORD_PAUSE_MULTIPLIER;
}

void MorseTimings::adjust(unsigned length, char reference[MORSE_SEQUENCE_MAX],
                          millis32_t new_timings[MORSE_SEQUENCE_MAX]) {
  millis32_t sum = 0;
  unsigned c = 0;

  for (unsigned i = 0; i < length; i++) {
    if (reference[i] == DIT) {
      sum += new_timings[i];
      c++;
    }
  }

  if (c) {
    this->set_dit_avg(sum / c);
  }
}
