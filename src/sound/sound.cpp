#include "sound.h"
#include "M5Unified.hpp"
#include "driver/ledc.h"
#include "hal/ledc_types.h"

#define GPIO_OUTPUT 13
#define GPIO_OUTPUT_SPEED LEDC_HIGH_SPEED_MODE

void _sound_setup(int gpio_num, ledc_timer_t timer, ledc_channel_t channel,
                  uint32_t freq) {

  ledc_timer_config_t timer_conf = {
      .speed_mode = GPIO_OUTPUT_SPEED,
      .duty_resolution = LEDC_TIMER_10_BIT,
      .timer_num = timer,
      .freq_hz = freq,
      .clk_cfg = LEDC_USE_REF_TICK,
      .deconfigure = false,
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t ledc_conf = {
      .gpio_num = gpio_num,
      .speed_mode = GPIO_OUTPUT_SPEED,
      .channel = channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0x0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
      .flags =
          {
              .output_invert = 0,
          },
  };
  ledc_channel_config(&ledc_conf);
}

void sound_setup(void) {
  const uint32_t freq = 660;

  switch (M5.getBoard()) {
  case m5::board_t::board_M5StickCPlus:
  case m5::board_t::board_M5StickCPlus2:
    _sound_setup(26 /* GPIO_PIN_REG_26 */, LEDC_TIMER_0, LEDC_CHANNEL_0, freq);
    _sound_setup(2 /* GPIO_PIN_REG_2 */, LEDC_TIMER_1, LEDC_CHANNEL_1, freq);
    break;
  case m5::board_t::board_M5StickC:
    _sound_setup(26 /* GPIO_PIN_REG_26 */, LEDC_TIMER_0, LEDC_CHANNEL_0, freq);
    break;
  default:
    break;
  }
}

void _sound_start(ledc_channel_t channel) {
  ledc_set_duty(GPIO_OUTPUT_SPEED, channel, 0x7F);
  ledc_update_duty(GPIO_OUTPUT_SPEED, channel);
}

void _sound_stop(ledc_channel_t channel) {
  ledc_set_duty(GPIO_OUTPUT_SPEED, channel, 0);
  ledc_update_duty(GPIO_OUTPUT_SPEED, channel);
}

void sound_start(void) {
  switch (M5.getBoard()) {
  case m5::board_t::board_M5StickCPlus:
  case m5::board_t::board_M5StickCPlus2:
    _sound_start(LEDC_CHANNEL_0);
    _sound_start(LEDC_CHANNEL_1);
    break;
  case m5::board_t::board_M5StickC:
    _sound_start(LEDC_CHANNEL_0);
    break;
  default:
    break;
  }
};

void sound_stop(void) {
  switch (M5.getBoard()) {
  case m5::board_t::board_M5StickCPlus:
  case m5::board_t::board_M5StickCPlus2:
    _sound_stop(LEDC_CHANNEL_0);
    _sound_stop(LEDC_CHANNEL_1);
    break;
  case m5::board_t::board_M5StickC:
    _sound_stop(LEDC_CHANNEL_0);
    break;
  default:
    break;
  }
};
