#ifndef LED_BAR_H
#define LED_BAR_H

#include <stdint.h>
#include <stdbool.h>

#define LED_LOGICAL_COUNT 10

typedef enum {
    LED_COLOR_RED = 0,
    LED_COLOR_GREEN,
    LED_COLOR_YELLOW,
} led_color_t;

void led_bar_init(void);

// Set what the bar should display (10-bit logical mask).
void led_bar_set_mask10(uint16_t mask10, led_color_t color);

// ISR tick (1 ms) for scanning
void led_bar_isr_scan_1ms(void);

#endif // LED_BAR_H
