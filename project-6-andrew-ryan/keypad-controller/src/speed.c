#include <msp430fr2310.h>
#include <stdbool.h>
#include <stdint.h>
#include "potentiometer.h"


volatile uint8_t  speed_pattern_tick= 0;

/*
 * speed of patterns math
 *
 * 16384 = 0.5 s
 *
 *
 *
 *
 *
 */

void timer_b0_init(void){

    TB0CTL |= TBCLR;
    TB0CTL |= TBSSEL__ACLK;
    TB0CTL |= MC__UP;
    TB0CTL |= TBCLR;
    TB0CCR0 = speedOfPatterns;  //speed of the patterns for the LEDs

    TB0CCTL0 &= ~CCIFG;
    TB0CCTL0 |= CCIE;

}



#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_B0_CCR0(void)
{

    //TODO: call led bar pattern so it will go one time
    speed_pattern_tick = 1;

    TB0CCTL0 &= ~CCIFG;    // clear CCR0 flag

}
