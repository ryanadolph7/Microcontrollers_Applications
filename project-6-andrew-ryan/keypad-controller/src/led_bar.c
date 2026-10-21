#include <msp430fr2310.h>
#include <stdbool.h>
#include <stdint.h>
#include "potentiometer.h"

#define NUM_LEDS        10
#define BYTES_PER_LED   9
#define RESET_BYTES     20

uint8_t spi_buf[NUM_LEDS * BYTES_PER_LED + RESET_BYTES];

volatile int red_amount = 1;
volatile int blue_amount = 1;
volatile int green_amount = 1;
volatile int button_amount_color = 0;
volatile int button_amount_pattern = 0;
volatile int pattern = 0;

static void wait_steps_10ms(uint16_t steps_10ms)
{
    uint16_t start = tick10ms;
    while ((uint16_t)(tick10ms - start) < steps_10ms) {

    }
}

/* Map speedOfPatterns (0..15) -> 0.1..1.0s as 10..100 steps of 10ms */
static inline uint16_t pattern_delay_steps_10ms(void)
{
    uint16_t steps = 10u + ((uint16_t)speedOfPatterns * 6u); // 10..100
    if (steps < 10u)  steps = 10u;
    if (steps > 100u) steps = 100u;
    return steps;
}


void encode_byte(uint8_t ws_byte, uint8_t *out) {
    uint8_t bit;
    uint32_t encoded = 0;

    for (bit = 0; bit < 8; bit++) {
        encoded <<= 3;
        if (ws_byte & 0x80)
            encoded |= 0x06;  // 110 = WS2812B '1'
        else
            encoded |= 0x04;  // 100 = WS2812B '0'
        ws_byte <<= 1;
    }

    out[0] = (uint8_t)(encoded >> 16);
    out[1] = (uint8_t)(encoded >> 8);
    out[2] = (uint8_t)(encoded);
}

void set_led(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t *p = &spi_buf[index * BYTES_PER_LED];
    encode_byte(green, p);        // WS2812B is GRB order
    encode_byte(red, p + 3);
    encode_byte(blue, p + 6);
}

void send_leds(void) {
    uint16_t i;
    for (i = 0; i < sizeof(spi_buf); i++) {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = spi_buf[i];
    }
    while (UCA0STATW & UCBUSY);
}

void set_all_green(void){

    int j;
    for (j = 0; j < NUM_LEDS; j++) {
         set_led(j, red_amount, green_amount, blue_amount);
    }
}
void led_bar_init(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    //Used CLAUDE to init clock


    //-- 1. Set DCO to 8 MHz (FR2153 CS module - NO password required)
    // CSCTL1: DCORSEL bits [3:1] = 011b selects 8 MHz range
    CSCTL1 = DCORSEL_3;

    // CSCTL2: FLLD [14:12] = 000b (divide by 1), FLLN [9:0] = 244
    // fDCO = (FLLN + 1) * fREFO = 245 * 32768 = ~8.028 MHz
    CSCTL2 = FLLD__1 | 244;

    // CSCTL3: SELREF [5:4] = 01b => use REFO (32.768 kHz) as FLL reference
    CSCTL3 = SELREF__REFOCLK;

    // CSCTL4: SELMS [2:0] = 000b => MCLK and SMCLK sourced from DCOCLKDIV
    // Default is already 000b, but set explicitly for clarity
    CSCTL4 &= ~SELMS_7;          // Clear SELMS bits => select DCOCLKDIV

    // Wait for FLL to lock (DCO to stabilize at 8 MHz)
    // Check FLLUNLOCK bits in CSCTL7
    while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));
    // CSCTL5: All dividers default to divide-by-1, no changes needed

    // Configure P4.1 = UCA1CLK, P4.3 = UCA1SIMO
    P1SEL1 &= ~(BIT2 | BIT5);
    P1SEL0 |=  (BIT2 | BIT5);

    /*/Port interrupt for buttons P3.1 for color P3.2 for pattern
    P3DIR &= ~ (BIT1 | BIT2);
    P3REN |= BIT1 | BIT2;
    P3OUT |= BIT1 | BIT2;
    P3IES |= BIT1 | BIT2;

    //enable IRQs
    P3IFG &= ~(BIT1 | BIT2);
    P3IE |= BIT1 | BIT2;

    __enable_interrupt();
    */
    UCA0CTLW0 |= UCSWRST;

    UCA0CTLW0 |= UCMST;               // Master mode
    UCA0CTLW0 |= UCSYNC;              // synchronous mode
    UCA0CTLW0 |= UCMSB;               // MSB first
    UCA0CTLW0 &= ~UC7BIT;             // 8-bit data
    UCA0CTLW0 &= ~UCMODE_3;           // 3-pin SPI (no STE)
    UCA0CTLW0 &= ~UCCKPL;             // Clock idle LOW (SPI mode 0)
    UCA0CTLW0 &= ~UCCKPH;             // Capture on first edge
    UCA0CTLW0 |= UCSSEL__SMCLK;       // Clock source = SMCLK (~8 MHz)

    UCA0BRW = 3;                       // ~8 MHz / 3 = 2.667 MHz SPI

    UCA0CTLW0 &= ~UCSWRST;

    // Zero out reset bytes
    uint8_t i;
    for (i = 0; i < RESET_BYTES; i++)
        spi_buf[NUM_LEDS * BYTES_PER_LED + i] = 0x00;

    set_all_green();

    // Send to strip

    send_leds();


}
void led_bar_step(void){
    switch(pattern){
        case 0:
            toggle_pattern0();
            break;
        case 1:
            upCounter_pattern1();
            break;
        case 2:
            inAndOut_pattern2();
            break;
        case 3:
            downCounter_pattern3();
            break;
        case 4:
            rotateOneLeft_pattern4();
            break;
        case 5:
            fillToRight_pattern5();
            break;

}
send_leds();
}

//pattern 0 (toggle)
//step 1: 1010101010
//step 2: 0101010101
void toggle_pattern0(void){
    int i;
    for (i = 0; i < NUM_LEDS; i++) {
        if (i % 2 == 0)
            set_led(i, red_amount, green_amount, blue_amount);  // Even LEDs = on
        else
            set_led(i, 0, 0, 0);    // Odd LEDs = OFF
    }
    send_leds();


    wait_steps_10ms(pattern_delay_steps_10ms());

    for (i = 0; i < NUM_LEDS; i++) {
        if (i % 2 == 0)

            set_led(i, 0, 0, 0);    // Odd LEDs = OFF
        else
            set_led(i, red_amount, green_amount, blue_amount);  // Even LEDs = on
    }
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

}


//pattern 1 (up counter)
//step 1: 0000000000
//step 2: 0000000001
//step 3: 0000000010
void upCounter_pattern1(void){
    //step 1
    int i;
    for(i = 0; i < NUM_LEDS; i++){
        set_led(i,0,0,0);
    }




}

//pattern 2 (in and out)
//step 1: 0000110000
//step 2: 0001001000
//step 3: 0010000100
//step 4: 0100000010
void inAndOut_pattern2(void){
    int i;
    for(i = 0; i < NUM_LEDS; i++){
        set_led(i,0,0,0);
    }
    //step 1
    set_led(4,red_amount,green_amount,blue_amount);
    set_led(5,red_amount,green_amount,blue_amount);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());
    //step 2
    set_led(3,red_amount,green_amount,blue_amount);
    set_led(6,red_amount,green_amount,blue_amount);
    set_led(4,0,0,0);
    set_led(5,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());


    set_led(2,red_amount,green_amount,blue_amount);
    set_led(7,red_amount,green_amount,blue_amount);
    set_led(3,0,0,0);
    set_led(6,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(1,red_amount,green_amount,blue_amount);
    set_led(8,red_amount,green_amount,blue_amount);
    set_led(2,0,0,0);
    set_led(7,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(0,red_amount,green_amount,blue_amount);
    set_led(9,red_amount,green_amount,blue_amount);
    set_led(1,0,0,0);
    set_led(8,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());
    //finished with out now onto in pattern

    set_led(1,red_amount,green_amount,blue_amount);
    set_led(8,red_amount,green_amount,blue_amount);
    set_led(0,0,0,0);
    set_led(9,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(2,red_amount,green_amount,blue_amount);
    set_led(7,red_amount,green_amount,blue_amount);
    set_led(1,0,0,0);
    set_led(8,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(3,red_amount,green_amount,blue_amount);
    set_led(6,red_amount,green_amount,blue_amount);
    set_led(7,0,0,0);
    set_led(2,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());






}

//pattern 3(down counter)
//step 1: 1111111111
//step 2: 1111111110
//step 3: 1111111101
void downCounter_pattern3(void){



}

//pattern 4 (rotate one left)
//step 1: 0000000001
//step 2: 0000000010
//step 3: 0000000100
void rotateOneLeft_pattern4(void){
    int i;
    for(i = 0; i < NUM_LEDS; i++){
        set_led(i,0,0,0);
    }
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());
    //todo: add wait time
    set_led(9, red_amount,green_amount,blue_amount);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());


    set_led(8,red_amount, green_amount, blue_amount);
    set_led(9,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(7,red_amount, green_amount, blue_amount);
    set_led(8,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(6,red_amount, green_amount, blue_amount);
    set_led(7,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(5,red_amount, green_amount, blue_amount);
    set_led(6,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(4,red_amount, green_amount, blue_amount);
    set_led(5,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(3,red_amount, green_amount, blue_amount);
    set_led(4,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(2,red_amount, green_amount, blue_amount);
    set_led(3,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(1,red_amount, green_amount, blue_amount);
    set_led(2,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(0,red_amount, green_amount, blue_amount);
    set_led(1,0,0,0);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

}

//pattern 5 (fill to right)
//step 1: 1000000000
//step 2: 1100000000
void fillToRight_pattern5(void){

    int i;
    for(i = 0; i < NUM_LEDS; i++){
        set_led(i,0,0,0);
    }
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(0, red_amount, green_amount, blue_amount);
    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(1, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(2, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());


    set_led(3, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(4, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(5, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(6, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());


    set_led(7, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());


    set_led(8, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

    set_led(9, red_amount, green_amount, blue_amount);

    send_leds();
    wait_steps_10ms(pattern_delay_steps_10ms());

}

void pattern6_Brightness(void){
    int Val1 = ADC_VALUE;







}
/*
#pragma vector = PORT3_VECTOR
__interrupt void ISR_PORT3_S1(void){

    if (P3IFG & BIT1){
        P3IFG &= ~BIT1;

        if (!(P3IN & BIT1))             // button still pressed (active-low)
            if(button_amount_color == 0){
                red_amount = 1;
                green_amount = 0;
                blue_amount = 0;
            }
            else if(button_amount_color == 1){
                red_amount = 0;
                green_amount = 1;
                blue_amount = 0;
            }
            else if(button_amount_color == 2){
                red_amount =0;
                green_amount = 0;
                blue_amount = 1;
            }
            else {
                red_amount = 1;
                green_amount = 1;
                blue_amount = 1;
            }
        button_amount_color++;
    }
    if (P3IFG & BIT2){
        P3IFG &= ~BIT2;

        if (!(P3IN & BIT2))             // button still pressed (active-low)
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
                pattern = 6;
            }
        button_amount_pattern++;

    }
    if (button_amount_color >= 4) button_amount_color = 0;


    if (button_amount_pattern >= 7) button_amount_pattern = 0;

    P3IFG &= ~BIT2;
    P3IFG &= ~BIT1;
}

*/




