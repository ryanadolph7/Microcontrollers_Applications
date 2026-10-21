#ifndef LED_BAR_H_
#define LED_BAR_H_

#include <msp430fr2153.h>
#include <stdint.h>

void LED_bar_init(void);
void timer_b0_init(void);
void led_bar_step(void);

extern volatile uint8_t pattern;
extern volatile uint8_t button_amount_pattern;
extern volatile uint16_t value;
extern volatile uint8_t toggle_count;
extern volatile uint8_t step_in_n_out;
extern volatile uint8_t step_rotate_left;
extern volatile uint8_t step_fill_right;
extern volatile uint16_t speed_of_patterns;
extern volatile uint8_t led_step_flag;

#endif /* LED_BAR_H_ */
