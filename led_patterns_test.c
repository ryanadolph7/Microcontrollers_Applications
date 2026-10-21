#include <msp430fr2153.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../src/led_patterns.h"
#include "../src/led_bar.h"

led_bar_t led_bar = {
    {.port_dir = P4_BASE + OFS_P4DIR, .port_out = P4_BASE + OFS_P4OUT, .pin = BIT6},
    {.port_dir = (uint16_t)&P4DIR, .port_out = (uint16_t)&P4OUT, .pin = BIT1},
    {.port_dir = (uint16_t)&P4DIR, .port_out = (uint16_t)&P4OUT, .pin = BIT0},
    {.port_dir = (uint16_t)&P2DIR, .port_out = (uint16_t)&P2OUT, .pin = BIT3},
    {.port_dir = (uint16_t)&P2DIR, .port_out = (uint16_t)&P2OUT, .pin = BIT2},
    {.port_dir = (uint16_t)&P2DIR, .port_out = (uint16_t)&P2OUT, .pin = BIT1},
    {.port_dir = (uint16_t)&P2DIR, .port_out = (uint16_t)&P2OUT, .pin = BIT0},
    {.port_dir = (uint16_t)&P1DIR, .port_out = (uint16_t)&P1OUT, .pin = BIT7},
    {.port_dir = (uint16_t)&P1DIR, .port_out = (uint16_t)&P1OUT, .pin = BIT6},
    {.port_dir = (uint16_t)&P1DIR, .port_out = (uint16_t)&P1OUT, .pin = BIT5},
};

struct led_pattern led_pattern;

void test_set_pattern()
{
    led_patterns_set_pattern(&led_pattern, TOGGLE);
    for (uint8_t i = 0; i < 20; i++)
    {
        led_patterns_update(&led_pattern);
        led_bar_write(led_bar, led_pattern.value);
        __delay_cycles(250000);
    }

    led_patterns_set_pattern(&led_pattern, UP_COUNTER);
    for (uint16_t i = 0; i < 20; i++)
    {
        led_patterns_update(&led_pattern);
        led_bar_write(led_bar, led_pattern.value);
        __delay_cycles(50000);
    }

    led_patterns_set_pattern(&led_pattern, IN_OUT);
    for (uint8_t i = 0; i < 20; i++)
    {
        led_patterns_update(&led_pattern);
        led_bar_write(led_bar, led_pattern.value);
        __delay_cycles(250000);
    }

    led_patterns_set_pattern(&led_pattern, DOWN_COUNTER);
    for (uint16_t i = 0; i < 20; i++)
    {
        led_patterns_update(&led_pattern);
        led_bar_write(led_bar, led_pattern.value);
        __delay_cycles(50000);
    }

    led_patterns_set_pattern(&led_pattern, ROTATE_LEFT);
    for (uint8_t i = 0; i < 20; i++)
    {
        led_patterns_update(&led_pattern);
        led_bar_write(led_bar, led_pattern.value);
        __delay_cycles(250000);
    }

    led_patterns_set_pattern(&led_pattern, FILL_RIGHT);
    for (uint8_t i = 0; i < 20; i++)
    {
        led_patterns_update(&led_pattern);
        led_bar_write(led_bar, led_pattern.value);
        __delay_cycles(250000);
    }
}

void test_next_pattern()
{
    for (uint8_t i = 0; i < 12; i++)
    {
        led_patterns_next_pattern(&led_pattern);
        for (uint8_t j = 0; j < 20; j++)
        {
            led_patterns_update(&led_pattern);
            led_bar_write(led_bar, led_pattern.value);
            __delay_cycles(100000);
        }
    }
}

void test_previous_pattern()
{
    for (uint8_t i = 0; i < 12; i++)
    {
        led_patterns_previous_pattern(&led_pattern);
        for (uint8_t j = 0; j < 20; j++)
        {
            led_patterns_update(&led_pattern);
            led_bar_write(led_bar, led_pattern.value);
            __delay_cycles(100000);
        }
    }
}

void main()
{

    // Stop the watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // Disable low-power mode
    PM5CTL0 &= ~LOCKLPM5;

    led_bar_init(led_bar);

    while (true)
    {
        test_set_pattern();

        led_bar_write(led_bar, 0);
        __delay_cycles(1000000);

        test_next_pattern();

        led_bar_write(led_bar, 0);
        __delay_cycles(1000000);

        test_previous_pattern();

        led_bar_write(led_bar, 0);
        __delay_cycles(1000000);
    }

}
