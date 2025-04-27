#include "morse.hpp"
#include <cstring>

typedef struct Morse {
  char alpha_num;
  char code[MORSE_SEQUENCE_MAX];
} Morse;

const static Morse MorseABC[] = {
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

unsigned len(const char *ptr) {
  unsigned i = 0;

  while (*(ptr + i) != '\0') {
    i++;
  }

  return i;
}

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

unsigned morse_get_code_threshold() {
  const unsigned values = sizeof(MorseABC) / sizeof(MorseABC[0]);
  unsigned max_code_len = 0;

  for (unsigned i = 0; i < values; i++) {
    unsigned seq_len = len(MorseABC[i].code);

    max_code_len = MAX(seq_len, max_code_len);
  }

  return max_code_len;
}

MorseSequence::MorseSequence() {
  this->morse_code_threshold = morse_get_code_threshold();
}

bool MorseSequence::is_empty() { return this->len == 0; }

bool MorseSequence::is_valid_sequence() {
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

char MorseSequence::letter() { return this->last_letter; }

void MorseSequence::push(char letter, SignalSilent *interval) {
  this->code[this->len] = letter;
  this->interval[this->len] = *interval;
  this->len++;
}

void MorseSequence::done() { this->clear(); }

void MorseSequence::clear() {
  for (; this->len > 0; this->len--) {
    this->code[this->len] = '\0';
    this->interval[this->len] = SignalSilent{};
  }
}
