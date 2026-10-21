#include <stdint.h>
#include <stdbool.h>

#include "led_patterns.h"

static volatile led_pattern_t g_pat = LED_PATTERN_0_TOGGLE;
static volatile uint16_t g_mask10 = 0;
static volatile uint32_t g_ms_accum = 0;

static uint16_t g_counter10 = 0;
static uint8_t  g_counter8  = 255;
static uint8_t  g_step      = 0;

#define BM(i) ((uint16_t)(1u << (i)))

static const uint16_t PAT2_SEQ[] = {
    BM(4) | BM(5),
    BM(3) | BM(6),
    BM(2) | BM(7),
    BM(1) | BM(8),
    BM(0) | BM(9),
    BM(1) | BM(8),
    BM(2) | BM(7),
    BM(3) | BM(6),
};
#define PAT2_LEN ((uint8_t)(sizeof(PAT2_SEQ)/sizeof(PAT2_SEQ[0])))

static uint32_t period_ms_for_pattern(led_pattern_t p)
{
    switch (p) {
        case LED_PATTERN_0_TOGGLE:      return 1000u;
        case LED_PATTERN_1_UP:          return 500u;
        case LED_PATTERN_2_INOUT:       return 500u;
        case LED_PATTERN_3_DOWN:        return 250u;
        case LED_PATTERN_4_ROTATE_LEFT: return 750u;
        case LED_PATTERN_5_FILL_RIGHT:  return 1000u;
        default:                        return 500u;
    }
}

static void reset_state(void)
{
    g_ms_accum = 0;
    g_step = 0;

    switch (g_pat) {
        case LED_PATTERN_0_TOGGLE: g_mask10 = 0b0101010101u; break;
        case LED_PATTERN_1_UP:     g_counter10 = 0; g_mask10 = g_counter10 & 0x03FFu; break;
        case LED_PATTERN_2_INOUT:  g_mask10 = PAT2_SEQ[0]; break;
        case LED_PATTERN_3_DOWN:   g_counter8 = 255; g_mask10 = (uint16_t)g_counter8; break;
        case LED_PATTERN_4_ROTATE_LEFT: g_mask10 = BM(9); break;
        case LED_PATTERN_5_FILL_RIGHT:  g_mask10 = BM(0); break;
        default: g_mask10 = 0; break;
    }
}

void led_patterns_init(void)
{
    g_pat = LED_PATTERN_0_TOGGLE;
    reset_state();
}

void led_patterns_next(void)
{
    g_pat = (led_pattern_t)((uint8_t)g_pat + 1u);
    if (g_pat >= LED_PATTERN_COUNT) g_pat = LED_PATTERN_0_TOGGLE;
    reset_state();
}

static void step_pattern(void)
{
    switch (g_pat) {
        case LED_PATTERN_0_TOGGLE:
            g_mask10 ^= 0x03FFu;
            break;
        case LED_PATTERN_1_UP:
            g_counter10 = (uint16_t)((g_counter10 + 1u) & 0x03FFu);
            g_mask10 = g_counter10;
            break;
        case LED_PATTERN_2_INOUT:
            g_step++;
            if (g_step >= PAT2_LEN) g_step = 0;
            g_mask10 = PAT2_SEQ[g_step];
            break;
        case LED_PATTERN_3_DOWN:
            g_counter8 = (uint8_t)(g_counter8 - 1u);
            g_mask10 = (uint16_t)g_counter8;
            break;
        case LED_PATTERN_4_ROTATE_LEFT:
            if (g_mask10 == BM(0)) g_mask10 = BM(9);
            else g_mask10 >>= 1;
            break;
        case LED_PATTERN_5_FILL_RIGHT:
            if (g_mask10 == 0x03FFu) g_mask10 = BM(0);
            else {
                g_mask10 = (uint16_t)((g_mask10 << 1) | 1u);
                g_mask10 &= 0x03FFu;
            }
            break;
        default:
            break;
    }
}

bool led_patterns_on_tick_1ms(void)
{
    uint32_t period = period_ms_for_pattern(g_pat);

    g_ms_accum++;
    if (g_ms_accum < period) return false;

    g_ms_accum = 0;
    step_pattern();
    return true;
}

uint16_t led_patterns_get_mask10(void)
{
    return (uint16_t)(g_mask10 & 0x03FFu);
}
