#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "led_bar.h"

// Timing
#define LED_SCAN_HZ     1000UL
#define SMCLK_HZ        1000000UL   // update if you change clocks

static volatile uint8_t g_scan_seg = 0;
static volatile uint8_t g_seg_out[3] = {0,0,0}; // P4 output image for each segment
static volatile led_color_t g_color = LED_COLOR_GREEN;

static const uint8_t g_logical_to_physical[LED_LOGICAL_COUNT] = {
    0,1,2,3,4,5,6,7,8,9
};

static uint16_t logical10_to_physical12(uint16_t mask10)
{
    uint16_t pmask;
    uint8_t logical;

    pmask = 0;
    for (logical = 0; logical < LED_LOGICAL_COUNT; logical++) {
        if (mask10 & (1u << logical)) {
            uint8_t phys = g_logical_to_physical[logical];
            if (phys < 12) pmask |= (uint16_t)(1u << phys);
        }
    }
    return pmask;
}

static void rebuild_segment_out(uint16_t physical12, led_color_t color)
{
    uint8_t seg;

    for (seg = 0; seg < 3; seg++) {
        uint8_t out;
        uint8_t j;

        out = 0;
        for (j = 0; j < 4; j++) {
            uint8_t phys;
            bool on;

            phys = (uint8_t)(seg * 4u + j);
            on = (physical12 & (1u << phys)) != 0;

            if (!on) continue;

            if (color == LED_COLOR_RED || color == LED_COLOR_YELLOW) {
#if RED_NIBBLE_REVERSED
                out |= (uint8_t)(1u << (3u - j));   // red select lines reversed (your fix #3)
#else
                out |= (uint8_t)(1u << j);
#endif
            }
            if (color == LED_COLOR_GREEN || color == LED_COLOR_YELLOW) {
                out |= (uint8_t)(1u << (j + 4u));   // green select normal
            }
        }
        g_seg_out[seg] = out;
    }
}

static inline void scan_step(void)
{
    // Disable all segments to avoid ghosting
    LEDEN_PORT_OUT &= (uint8_t)~(LEDEN_LO_BIT | LEDEN_MID_BIT | LEDEN_HI_BIT);

    // Clear mux/color lines
    LEDMUX_PORT_OUT &= (uint8_t)~LEDMUX_ALL_MASK;

    // Drive desired lines
    LEDMUX_PORT_OUT |= g_seg_out[g_scan_seg];

    // Enable segment
    if (g_scan_seg == 0)      LEDEN_PORT_OUT |= LEDEN_LO_BIT;
    else if (g_scan_seg == 1) LEDEN_PORT_OUT |= LEDEN_MID_BIT;
    else                      LEDEN_PORT_OUT |= LEDEN_HI_BIT;

    g_scan_seg++;
    if (g_scan_seg >= 3) g_scan_seg = 0;
}

void led_bar_init(void)
{
    // Mux/color lines
    LEDMUX_PORT_DIR |= LEDMUX_ALL_MASK;
    LEDMUX_PORT_OUT &= (uint8_t)~LEDMUX_ALL_MASK;

    // Enables
    LEDEN_PORT_DIR |= (LEDEN_LO_BIT | LEDEN_MID_BIT | LEDEN_HI_BIT);
    LEDEN_PORT_OUT &= (uint8_t)~(LEDEN_LO_BIT | LEDEN_MID_BIT | LEDEN_HI_BIT);

    // TimerB0 for scan tick
    TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
    TB0CCR0 = (uint16_t)(SMCLK_HZ / LED_SCAN_HZ - 1u);
    TB0CCTL0 = CCIE;

    // default display off
    led_bar_set_mask10(0, LED_COLOR_GREEN);
}

void led_bar_set_mask10(uint16_t mask10, led_color_t color)
{
    g_color = color;
    rebuild_segment_out(logical10_to_physical12(mask10), color);
}

void led_bar_isr_scan_1ms(void)
{
    scan_step();
}
