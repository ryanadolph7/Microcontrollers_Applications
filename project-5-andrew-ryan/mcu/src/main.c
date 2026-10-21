#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "lcd_contrast.h"



int main(void) {
    // Stop watchdog timer and unlock GPIO (for MSP430FR2311).
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;
   
   
    // Initialize Timer_B before using the timer-based delay.
    timer_init();
   
    // Initialize the LCD.
    lcd_data_init();
    lcd_init();

    // Initialize the DAC 
    spi_init();
    //UCB1IFG &= ~UCTXIFG;        // enable UCTXIE interrupt anc clear TXIFG intially
    //UCB1IE |= UCTXIE;           // will trigger whenever there is a value in the change DAC (TX) buffer, should probably be sending all the time??

    __enable_interrupt();
   
    // Optionally, write a test message ("Hello") to the LCD.
    lcd_data('B');
    lcd_data('a');
    lcd_data('l');
    lcd_data('l');
    lcd_data('s');
    lcd_command(0x01); // clear the screen
    // Enable global interrupts. 
    while(true) {
        spi_write(1);
        write_all();
        
        //__delay_cycles(100000);
        //spi_write(125);               // used to test LCD contrast from DAC output 
        //__delay_cycles(100000);       // writing high value lowers LCD contrast 

        //lcd_data(0x7E);
        //for (i = 33; i < 0b10001111; i++){ 
            //lcd_command(0x01);
            //lcd_data(i);
            //__delay_cycles(500000);
        //}
        //for (i = 0b10100000; i <=11111111; i++) {
            //lcd_command(0x01);
            //lcd_data(i);
            //delay_cycles(500000);
        //}
        //delay_timer_ms(00);
        //__delay_cycles(500000);
        //spi_write(125);
        //__delay_cycles(500000);
        //delay_timer_ms(500);
        //lcd_data('!');
    }
   
    return 0;
}
