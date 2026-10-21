#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LED_PATTERN_0_TOGGLE = 0,
    LED_PATTERN_1_UP,
    LED_PATTERN_2_INOUT,
    LED_PATTERN_3_DOWN,
    LED_PATTERN_4_ROTATE_LEFT,
    LED_PATTERN_5_FILL_RIGHT,
    LED_PATTERN_COUNT
} led_pattern_t;

void led_patterns_init(void);
void led_patterns_next(void);

bool led_patterns_on_tick_1ms(void);
uint16_t led_patterns_get_mask10(void);

#endif
