#include <cstdint>

#define NO_SIGNAL '\0'
#define DIT '.'
#define DAH '_'

typedef char MorseSignal;

// Maximum length of sequence
#define MORSE_SEQUENCE_MAX 10

// > Medium gap (between words): seven time units long (formerly five)
// - https://en.wikipedia.org/wiki/Morse_code#Representation,_timing,_and_speeds
#define MORSE_WORD_MULTIPLIER 5

// > short gap (between letters): three time units long
// - https://en.wikipedia.org/wiki/Morse_code#Representation,_timing,_and_speeds
#define MORSE_LETTER_MULTIPLIER 3

// To distinguish DIT from DAH '2' is enough
// because 'DAH is three time units long'
// This value can use to distinguish
// letter pause from dit/dah pause
#define DIT_DAH_THRESHOLD_MULTIPLIER 2

// > Medium gap (between words): seven time units long (formerly five)
// so, to distinguish DIT and DATH from '4' is enough
#define WORD_PAUSE_MULTIPLIER 4

typedef struct Morse {
  char alpha_num;
  MorseSignal code[MORSE_SEQUENCE_MAX];
} Morse;

// 32-bits milliseconds
typedef uint32_t millis32_t;

class MorseTimings {
public:
  millis32_t dit_avg;
  millis32_t dit_dah_threshold;
  millis32_t word_pause_threshold;

  MorseTimings(millis32_t dit_avg);

  // True if `interval` is for dit
  bool is_dit(millis32_t interval);

  // True if `interval` is for dah
  bool is_dah(millis32_t interval);

  // True if interval is for letter pause
  bool is_letter(millis32_t interval);

  // True if interval is for word pause
  bool is_word(millis32_t interval);

  void set_dit_avg(millis32_t dit_avg);

  // Adjust DIT time
  void adjust(unsigned length, char reference[MORSE_SEQUENCE_MAX],
              millis32_t new_timings[MORSE_SEQUENCE_MAX]);
};

class MorseSequence {
private:
  // Maximum length of known sequences
  // drop letter if exceeded
  unsigned morse_code_threshold;

  /// Last founded code after `::is_valid_sequence()`
  const Morse *match;

  unsigned committed;

public:
  MorseSignal code[MORSE_SEQUENCE_MAX];
  millis32_t interval[MORSE_SEQUENCE_MAX];
  unsigned len;

  MorseSequence();

  // True if sequence is empty
  bool is_empty();

  // True if current sequence exist in ABC
  bool is_valid_sequence();

  // True if current sequence is fully complete
  bool is_complete_sequence();

  // callable only after `::is_valid_sequence()`
  char letter();

  // Propose signal to queue
  void signal_propose(MorseSignal signal, millis32_t interval);

  // Commit last signal with new interval
  void signal_commit();

  // Discard all uncommitted signals
  void signal_discard();

  void done();

  void clear();
};

extern void morse_sequence_setup();
