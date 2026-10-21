#include <msp430fr2310.h>
#include <stdbool.h>
#include <stdint.h>
#include "keypad_driver.h"
#include "keypad_uart.h"
#include "led_bar.h"
#include "potentiometer.h"
#include "speed.h"



/* Driver for Keypad MCU - MSP430FR2310 to send keypad input to main MCU (MSP430FR2355)
/   Baudrate = 115200
*/

int main(void)
{
    // Stop the watchdog timer
    WDTCTL = WDTPW | WDTHOLD;
    
    // Disable low-power mode
    PM5CTL0 &= ~LOCKLPM5;
    uart_init();
    keypad_init();
    __enable_interrupt();

    current_character = 0;
    col_1_press = 0;
    col_2_press = 0;
    col_3_press = 0;


    led_bar_init();
    potentiometer_init();
    timer_b0_init();


    uint16_t last_ticks = speedOfPatterns;


    while(true)
    {
        shift_high_row();
        __delay_cycles(100);                   
        if ((current_character != 0 && col_1_press == 1) || (current_character != 0 && col_2_press == 1) || (current_character != 0 && col_3_press == 1)) {
            uart_send_char(current_character);
            //uart_send_char('\n');               // used to debug in AD2, can probably take this out so we dont have to include parsing
            current_character = 0;
            col_1_press = 0;
            col_2_press = 0;
            col_3_press = 0;
        }
        if(speedOfPatterns != last_ticks){

            last_ticks = speedOfPatterns;
            TB0CCR0 = last_ticks;

        }
        if (speed_pattern_tick){
            speed_pattern_tick = 0;
            led_bar_step();
        }
       
    }
}
