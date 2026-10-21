#include <msp430.h>
#include <stdint.h>
#include "LED_bar.h"
#include "rtc.h"

#define LEDS_P3   (BIT3 | BIT4 | BIT5 | BIT6 | BIT7)   // 38,35,34,33,32
#define LEDS_P5   (BIT0 | BIT1)                        // 37,36
#define LEDS_P2   (BIT0 | BIT1 | BIT2)                 // 27,26,25

extern volatile uint8_t pattern = 0;
volatile uint8_t button_amount_pattern = 0;
volatile uint16_t value = 0;
volatile uint8_t toggle_count = 0;
volatile uint8_t step_in_n_out = 0;
volatile uint8_t step_rotate_left = 0;
volatile uint8_t step_fill_right = 0;
volatile uint16_t speed_of_patterns = 32768;
extern volatile uint8_t led_step_flag = 0;

volatile uint8_t rtc_mode = 0;
volatile uint8_t patterns_mode = 1;
volatile uint8_t rtc_display_index = 0;






static uint8_t bin_to_bcd_led(uint8_t value_in)
{
    return ((value_in / 10) << 4) | (value_in % 10);
}

static void display_led_bar_value(uint16_t display_value)
{
    P3OUT &= ~LEDS_P3;
    P5OUT &= ~LEDS_P5;
    P2OUT &= ~LEDS_P2;

    if (display_value & (1 << 0)) P3OUT |= BIT3;   // pin 38
    if (display_value & (1 << 1)) P5OUT |= BIT0;   // pin 37
    if (display_value & (1 << 2)) P5OUT |= BIT1;   // pin 36
    if (display_value & (1 << 3)) P3OUT |= BIT4;   // pin 35
    if (display_value & (1 << 4)) P3OUT |= BIT5;   // pin 34
    if (display_value & (1 << 5)) P3OUT |= BIT6;   // pin 33
    if (display_value & (1 << 6)) P3OUT |= BIT7;   // pin 32
    if (display_value & (1 << 7)) P2OUT |= BIT0;   // pin 27
    if (display_value & (1 << 8)) P2OUT |= BIT1;   // pin 26
    if (display_value & (1 << 9)) P2OUT |= BIT2;   // pin 25
}

static void display_rtc_register(void)
{
    uint16_t rtc_bcd_value = 0;

    if (!rtc_ok)
    {
        display_led_bar_value(0);
        return;
    }

    switch (rtc_display_index)
    {
        case 0:
            rtc_bcd_value = bin_to_bcd_led((uint8_t)(rtc_year % 100));
            break;
        case 1:
            rtc_bcd_value = bin_to_bcd_led(rtc_month);
            break;
        case 2:
            rtc_bcd_value = bin_to_bcd_led(rtc_day);
            break;
        case 3:
            rtc_bcd_value = bin_to_bcd_led(rtc_hour);
            break;
        case 4:
            rtc_bcd_value = bin_to_bcd_led(rtc_minute);
            break;
        case 5:
            rtc_bcd_value = bin_to_bcd_led(rtc_second);
            break;
        default:
            rtc_display_index = 0;
            rtc_bcd_value = bin_to_bcd_led((uint8_t)(rtc_year % 100));
            break;
    }

    display_led_bar_value(rtc_bcd_value);
}
static void init_leds(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog


    P3SEL0 &= ~LEDS_P3;
    P3SEL1 &= ~LEDS_P3;

    P5SEL0 &= ~LEDS_P5;
    P5SEL1 &= ~LEDS_P5;

    P2SEL0 &= ~LEDS_P2;
    P2SEL1 &= ~LEDS_P2;


    P3DIR |= LEDS_P3;
    P5DIR |= LEDS_P5;
    P2DIR |= LEDS_P2;


    P3OUT &= ~LEDS_P3;
    P5OUT &= ~LEDS_P5;
    P2OUT &= ~LEDS_P2;


    PM5CTL0 &= ~LOCKLPM5;



}

/*
 * 0 -> pin 38 -> P3.3
 * 1 -> pin 37 -> P5.0
 * 2 -> pin 36 -> P5.1
 * 3 -> pin 35 -> P3.4
 * 4 -> pin 34 -> P3.5
 * 5 -> pin 33 -> P3.6
 * 6 -> pin 32 -> P3.7
 * 7 -> pin 27 -> P2.0
 * 8 -> pin 26 -> P2.1
 * 9 -> pin 25 -> P2.2
 */
static uint16_t binary_up_counter(uint16_t value_up)
{
    // clear all LED bits
    P3OUT &= ~LEDS_P3;
    P5OUT &= ~LEDS_P5;
    P2OUT &= ~LEDS_P2;

    // set bits according to value
    if (value_up & (1 << 0)) P3OUT |= BIT3;   // pin 38
    if (value_up & (1 << 1)) P5OUT |= BIT0;   // pin 37
    if (value_up & (1 << 2)) P5OUT |= BIT1;   // pin 36
    if (value_up & (1 << 3)) P3OUT |= BIT4;   // pin 35
    if (value_up & (1 << 4)) P3OUT |= BIT5;   // pin 34
    if (value_up & (1 << 5)) P3OUT |= BIT6;   // pin 33
    if (value_up & (1 << 6)) P3OUT |= BIT7;   // pin 32
    if (value_up & (1 << 7)) P2OUT |= BIT0;   // pin 27
    if (value_up & (1 << 8)) P2OUT |= BIT1;   // pin 26
    if (value_up & (1 << 9)) P2OUT |= BIT2;   // pin 25

    value_up++;
    if (value_up > 1023){
        value_up = 0;
    }
    return value_up;
}

static uint16_t binary_down_counter(uint16_t value_down)
{
    // clear all LED bits
    P3OUT &= ~LEDS_P3;
    P5OUT &= ~LEDS_P5;
    P2OUT &= ~LEDS_P2;

    // set bits according to value
    if (value_down & (1 << 0)) P3OUT |= BIT3;   // pin 38
    if (value_down & (1 << 1)) P5OUT |= BIT0;   // pin 37
    if (value_down & (1 << 2)) P5OUT |= BIT1;   // pin 36
    if (value_down & (1 << 3)) P3OUT |= BIT4;   // pin 35
    if (value_down & (1 << 4)) P3OUT |= BIT5;   // pin 34
    if (value_down & (1 << 5)) P3OUT |= BIT6;   // pin 33
    if (value_down & (1 << 6)) P3OUT |= BIT7;   // pin 32
    if (value_down & (1 << 7)) P2OUT |= BIT0;   // pin 27
    if (value_down & (1 << 8)) P2OUT |= BIT1;   // pin 26
    if (value_down & (1 << 9)) P2OUT |= BIT2;   // pin 25


    if (value_down == 0 ){
        value_down = 1023;
    }
    else
    {
        value_down--;
    }
    return value_down;

}

static uint8_t toggle_LEDs(uint8_t toggle_count){

    //first init of LEDs to get 10101010


        if(toggle_count == 0) {//make the first pattern 1010101010
                P3OUT |= BIT3;
                P5OUT &= ~BIT0;
                P5OUT |= BIT1;
                P3OUT &= ~BIT4;
                P3OUT |= BIT5;
                P3OUT &= ~BIT6;
                P3OUT |= BIT7;
                P2OUT &= ~BIT0;
                P2OUT |= BIT1;
                P2OUT &= ~BIT2;
                toggle_count++;
        }
                else{//make next pattern 010101010101

                P3OUT &= ~BIT3;
                P5OUT |= BIT0;
                P5OUT &= ~BIT1;
                P3OUT |= BIT4;
                P3OUT &= ~BIT5;
                P3OUT |= BIT6;
                P3OUT &= ~BIT7;
                P2OUT |= BIT0;
                P2OUT &= ~BIT1;
                P2OUT |= BIT2;
                toggle_count--;

        }



        return toggle_count;

}

static uint8_t in_and_out(uint8_t step_in_n_out){

    // clear all LED bits
    P3OUT &= ~LEDS_P3;
    P5OUT &= ~LEDS_P5;
    P2OUT &= ~LEDS_P2;

    switch(step_in_n_out){
        case 0:
            P3OUT |= BIT5;
            P3OUT |= BIT6;
            break;
        case 1:
            P3OUT |= BIT7;
            P3OUT |= BIT4;
            break;
        case 2:
            P5OUT |= BIT1;
            P2OUT |= BIT0;
            break;
        case 3:
            P5OUT |= BIT0;
            P2OUT |= BIT1;
            break;
        case 4:
            P3OUT |= BIT3;
            P2OUT |= BIT2;
            break;
        case 5:
            P5OUT |= BIT0;
            P2OUT |= BIT1;
            break;
        case 6:
            P5OUT |= BIT1;
            P2OUT |= BIT0;
            break;
        case 7:
            P3OUT |= BIT7;
            P3OUT |= BIT4;
            break;

        default:
            step_in_n_out = 0;
            break;


    }


    step_in_n_out++;
    if(step_in_n_out > 7){
        step_in_n_out = 0;
    }
    return step_in_n_out;

}

static uint8_t rotate_left(uint8_t step_rotate_left){
    // clear all LED bits
        P3OUT &= ~LEDS_P3;
        P5OUT &= ~LEDS_P5;
        P2OUT &= ~LEDS_P2;

        switch(step_rotate_left){
            case 0:
                P2OUT |= BIT2;
                break;
            case 1:
                P2OUT |= BIT1;
                break;
            case 2:
                P2OUT |= BIT0;
                break;
            case 3:
                P3OUT |= BIT7;
                break;
            case 4:
                P3OUT |= BIT6;
                break;
            case 5:
                P3OUT |= BIT5;
                break;
            case 6:
                P3OUT |= BIT4;
                break;
            case 7:
                P5OUT |= BIT1;
                break;
            case 8:
                P5OUT |= BIT0;
                break;
            case 9:
                P3OUT |= BIT3;
                break;
            default:
                step_rotate_left = 0;
                break;

        }
        step_rotate_left++;
        if(step_rotate_left > 9){
            step_rotate_left = 0;
        }
        return step_rotate_left;
}
static uint8_t fill_right(uint8_t step_fill_right){
    // clear all LED bits
        P3OUT &= ~LEDS_P3;
        P5OUT &= ~LEDS_P5;
        P2OUT &= ~LEDS_P2;

        switch(step_fill_right){
            case 0:
                P3OUT |= BIT3;
                break;
            case 1:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                break;
            case 2:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                break;
            case 3:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                P3OUT |= BIT4;
                break;
            case 4:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                P3OUT |= BIT4;
                P3OUT |= BIT5;
                break;
            case 5:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                P3OUT |= BIT4;
                P3OUT |= BIT5;
                P3OUT |= BIT6;
                break;
            case 6:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                P3OUT |= BIT4;
                P3OUT |= BIT5;
                P3OUT |= BIT6;
                P3OUT |= BIT7;
                break;
            case 7:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                P3OUT |= BIT4;
                P3OUT |= BIT5;
                P3OUT |= BIT6;
                P3OUT |= BIT7;
                P2OUT |= BIT0;
                break;
            case 8:
                P3OUT |= BIT3;
                P5OUT |= BIT0;
                P5OUT |= BIT1;
                P3OUT |= BIT4;
                P3OUT |= BIT5;
                P3OUT |= BIT6;
                P3OUT |= BIT7;
                P2OUT |= BIT0;
                P2OUT |= BIT1;
                break;
            case 9:
                P3OUT |= BIT3;
                P3OUT |= BIT4;
                P3OUT |= BIT5;
                P3OUT |= BIT6;
                P3OUT |= BIT7;
                P5OUT |= BIT1;
                P5OUT |= BIT0;
                P2OUT |= BIT2;
                P2OUT |= BIT1;
                P2OUT |= BIT0;
                break;
            default:
                step_fill_right = 0;
                break;

        }
        step_fill_right++;
        if(step_fill_right > 9){
            step_fill_right = 0;
        }
        return step_fill_right;
}


void LED_bar_init(void)
{
    init_leds();

    P3DIR &= ~(BIT2 | BIT1);
    P3REN |= BIT2 | BIT1;
    P3OUT |= BIT2 | BIT1;
    P3IES |= BIT2 | BIT1;
    P3IFG &= ~(BIT2 | BIT1);
    P3IE  |= BIT2 | BIT1;

    timer_b0_init();
}


void timer_b0_init(void){

    TB0CTL |= TBCLR;
    TB0CTL |= TBSSEL__ACLK;
    TB0CTL |= MC__UP;
    TB0CCR0 = speed_of_patterns;  //speed of the patterns for the LEDs
    TB0CCTL0 |= CCIE;
    TB0CCTL0 &= ~CCIFG;


}

void led_bar_step(void){

            if (rtc_mode == 1)
            {
                speed_of_patterns = 32768;
                TB0CCR0 = speed_of_patterns;

                rtc_update_time();
                display_rtc_register();
                return;
            }

            switch(pattern){
                case 0:

                    speed_of_patterns = 32764;      // 1 second
                    TB0CCR0 = speed_of_patterns;
                    toggle_count = toggle_LEDs(toggle_count);

                    break;
                case 1:
                    speed_of_patterns = 16384;      // .5 second
                    TB0CCR0 = speed_of_patterns;

                    value = binary_up_counter(value);
                    break;
                case 2:
                    speed_of_patterns = 16384;      // .5 second
                    TB0CCR0 = speed_of_patterns;

                    step_in_n_out = in_and_out(step_in_n_out);
                    break;
                case 3:
                    speed_of_patterns = 8192;       // .25 seconds
                    TB0CCR0 = speed_of_patterns;

                    value = binary_down_counter(value);
                    break;
                case 4:
                    speed_of_patterns = 24576;      // .75 seconds
                    TB0CCR0 = speed_of_patterns;
                    step_rotate_left = rotate_left(step_rotate_left);
                    break;
                case 5:
                    speed_of_patterns = 32768;      // 1 second
                    TB0CCR0 = speed_of_patterns;
                    step_fill_right = fill_right(step_fill_right);
                    break;
                default:
                    pattern = 0;
                    break;
            }
}
#pragma vector = PORT3_VECTOR
__interrupt void ISR_PORT3_S1(void)
{
    if (P3IFG & BIT2)
    {
        P3IFG &= ~BIT2;

        if (!(P3IN & BIT2))
        {
            if (rtc_mode == 0)
            {
                if(button_amount_pattern == 0){
                    pattern = 0;
                }
                else if(button_amount_pattern == 1){
                    pattern = 1;
                }
                else if(button_amount_pattern == 2){
                    pattern = 2;
                }
                else if(button_amount_pattern == 3){
                    pattern = 3;
                }
                else if(button_amount_pattern == 4){
                    pattern = 4;
                }
                else if (button_amount_pattern == 5){
                    pattern = 5;
                }
                else {
                    pattern = 0;
                }

                button_amount_pattern++;

                if (button_amount_pattern >= 6){
                    button_amount_pattern = 0;
                }
            }
            else
            {
                rtc_display_index++;

                if (rtc_display_index >= 6){
                    rtc_display_index = 0;
                }

                rtc_update_time();
                display_rtc_register();
            }
        }
    }

    if (P3IFG & BIT1)
    {
        P3IFG &= ~BIT1;

        if (!(P3IN & BIT1))
        {
            if(rtc_mode == 1){
                rtc_mode = 0;
                patterns_mode = 1;
            }
            else{
                rtc_mode = 1;
                patterns_mode = 0;
                rtc_display_index = 0;
                rtc_update_time();
                display_rtc_register();
            }
        }
    }
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_B0_CCR0(void)
{

    //TODO: call led bar pattern so it will go one time
    TB0CCTL0 &= ~CCIFG;    // clear CCR0 flag
    led_step_flag = 1;

}


