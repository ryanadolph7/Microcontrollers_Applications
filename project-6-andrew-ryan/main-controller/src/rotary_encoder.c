#include <msp430fr2153.h>
#include <stdint.h>
#include "rotary_encoder.h"

volatile uint8_t encoder_output = ENC_NONE;

volatile uint8_t enc_debug_flags = 0;
volatile uint8_t enc_debug_last_ab = 0;
volatile uint8_t enc_debug_transition = 0;
volatile int8_t  enc_debug_accum = 0;

static uint8_t last_ab = 0;
static int8_t step_accum = 0;

static inline uint8_t read_ab(void)
{
    uint8_t ab = 0;

    if (P2IN & ENC_A) ab |= 0x02;
    if (P2IN & ENC_B) ab |= 0x01;

    return ab;
}

static inline void update_ab_edge_select(void)
{
    if (P2IN & ENC_A)
        P2IES |= ENC_A;
    else
        P2IES &= ~ENC_A;

    if (P2IN & ENC_B)
        P2IES |= ENC_B;
    else
        P2IES &= ~ENC_B;
}

static int8_t transition_delta(uint8_t prev, uint8_t curr)
{
    switch ((prev << 2) | curr)
    {
        case 0x0D:
        case 0x04:
        case 0x02:
        case 0x0B:
            return 1;

        case 0x0E:
        case 0x08:
        case 0x01:
        case 0x07:
            return -1;

        case 0x00:
        case 0x05:
        case 0x0A:
        case 0x0F:
            return 0;

        default:
            return 2;
    }
}

void init_rotary_encoder(void)
{
    P2DIR &= ~(ENC_A | ENC_B | ENC_SW);

    P2SEL0 &= ~(ENC_A | ENC_B | ENC_SW);
    P2SEL1 &= ~(ENC_A | ENC_B | ENC_SW);

    P2REN &= ~(ENC_A | ENC_B | ENC_SW);
    P2OUT &= ~(ENC_A | ENC_B | ENC_SW);

    encoder_output = ENC_NONE;
    last_ab = read_ab();
    step_accum = 0;

    enc_debug_flags = 0;
    enc_debug_last_ab = last_ab;
    enc_debug_transition = 0;
    enc_debug_accum = 0;

    update_ab_edge_select();

    P2IES &= ~ENC_SW;

    P2IFG &= ~(ENC_A | ENC_B | ENC_SW);
    P2IE  |=  (ENC_A | ENC_B | ENC_SW);

    TB1CCR0  = 655;
    TB1CCTL0 = CCIE;
    TB1CTL   = TBSSEL__ACLK | MC__STOP | TBCLR;

    //__enable_interrupt();
}

#pragma vector = PORT2_VECTOR
__interrupt void Port_2_ISR(void)
{
    uint8_t flags = P2IFG & P2IE;
    enc_debug_flags = flags;

    if (flags & (ENC_A | ENC_B))
    {
        uint8_t curr_ab = read_ab();
        int8_t delta = transition_delta(last_ab, curr_ab);

        enc_debug_transition = (last_ab << 2) | curr_ab;
        last_ab = curr_ab;
        enc_debug_last_ab = curr_ab;

        if (delta == 1 || delta == -1)
        {
            step_accum += delta;
        }
        else if (delta == 2)
        {
            step_accum = 0;
        }

        enc_debug_accum = step_accum;

        if (step_accum >= ENC_STEPS_PER_EVENT)
        {
            encoder_output = ENC_CW;
            step_accum = 0;
            enc_debug_accum = 0;
            __bic_SR_register_on_exit(LPM0_bits);
        }
        else if (step_accum <= -ENC_STEPS_PER_EVENT)
        {
            encoder_output = ENC_CCW;
            step_accum = 0;
            enc_debug_accum = 0;
            __bic_SR_register_on_exit(LPM0_bits);
        }

        update_ab_edge_select();
        P2IFG &= ~(ENC_A | ENC_B);
    }

    if (flags & ENC_SW)
    {
        P2IE  &= ~ENC_SW;
        P2IFG &= ~ENC_SW;

        TB1CTL = TBSSEL__ACLK | MC__UP | TBCLR;
    }
}

#pragma vector = TIMER1_B0_VECTOR
__interrupt void Timer1_B0_ISR(void)
{
    TB1CTL = TBSSEL__ACLK | MC__STOP | TBCLR;
    TB1CCTL0 &= ~CCIFG;

    if (P2IN & ENC_SW)
    {
        encoder_output = ENC_BUTTON;
        __bic_SR_register_on_exit(LPM0_bits);
    }

    P2IES &= ~ENC_SW;
    P2IFG &= ~ENC_SW;
    P2IE  |= ENC_SW;
}
