#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "app.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    PM5CTL0 &= ~LOCKLPM5;

    app_init();

    __enable_interrupt();

    while (1) {
        app_task();
        __bis_SR_register(LPM0_bits | GIE);
        __no_operation();
    }
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void)
{
    if (app_isr_tick_1ms()) {
        __bic_SR_register_on_exit(LPM0_bits);
    }
    TB0CCTL0 &= ~CCIFG;
}

#pragma vector = PORT3_VECTOR
__interrupt void PORT3_ISR(void)
{
    if (app_isr_port3()) {
        __bic_SR_register_on_exit(LPM0_bits);
    }
    // app_isr_port3 clears IFGs it handles
}
