#include <msp430fr2153.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "temp_sensor.h"
#include "lcd_contrast.h"
#include "uart_driver.h"
#include "rotary_encoder.h"
#include "rtc.h"
#include "LED_bar.h"


volatile int encoder_position = 0;
volatile uint8_t last_event = ENC_NONE;
volatile uint16_t button_presses = 0;



int current_screen = 0;
//int current_row = 0;

int main(void) {
    // Stop watchdog timer and unlock GPIO (for MSP430FR2311).
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;
    __enable_interrupt();
   
    //timer_init();
    lcd_data_init();
    lcd_init();
    spi_init();
    uart_init();
    temp_sensor_init();
    rtc_init();
    LED_bar_init();



    rtc_set_time_simple(2026, 4, 14, 2, 19, 0, 2);

    //UCB1IFG &= ~UCTXIFG;        // enable UCTXIE interrupt anc clear TXIFG intially
    //UCB1IE |= UCTXIE;           // will trigger whenever there is a value in the change DAC (TX) buffer, should probably be sending all the time??

    init_rotary_encoder();
    spi_write(LCD_CONT);

    while (true)
    {
        //__bis_SR_register(LPM0_bits | GIE);
        if (current_first_row > 3) {        // if we scroll down too low, keep it at the very last row
            current_first_row = 3;
        } 
        if (current_first_row < 0) {        // if we scroll up too much, keep it at the very first row
            current_first_row = 0;
        }
        switch (encoder_output)
        {
            case ENC_CW:
                encoder_position++;
                last_event = ENC_CW;
                encoder_output = ENC_NONE;
                //current_first_row--;
                lcd_command(0x01);
                if (current_row <= 0 && current_first_row > 0) {
                    current_first_row--;
                    current_row = 0;
                } else {
                    current_row--;
                }
                break;

            case ENC_CCW:
                encoder_position--;
                last_event = ENC_CCW;
                encoder_output = ENC_NONE;
                //current_first_row++;
                //if (current_first_row >= 3) {
                    //current_first_row = 3;
                //}
                lcd_command(0x01);
                if (current_row  >= 3 && current_first_row < 3) {
                    current_first_row++;
                    current_row = 3;
                } else {
                    current_row++;
                }
                break;

            case ENC_BUTTON:
                button_presses++;
                last_event = ENC_BUTTON;
                encoder_output = ENC_NONE;
                int selected_index = current_row + current_first_row;
                lcd_command(0x01);
                switch (current_screen)
                {
                    case 1: // MAIN MENU
                        switch (selected_index)
                        {
                            case 0: // "Main screen"
                                current_screen = 0;
                                break;

                            case 1: // "Window size"
                                
                                break;

                            case 2: // "LCD contrast"
                            
                                break;

                            case 3: // "RTC settings"
                                current_screen = 3;
                                current_row = 0;
                                current_first_row = 0;
                                break;

                            case 4: // "WS2812B color"
                                current_screen = 2;
                                current_row = 0;
                                current_first_row = 0;
                                break;

                            case 5: // "Cursor"
                                CURSOR_ON ^= 1;
                                //if (CURSOR_ON == 1) {
                                    //lcd_command(0x0E);
                                //} else {
                                    //lcd_command(0x0C);
                                //}
                                break;

                            case 6: // "Blink"
                                BLINK_ON ^= 1;
                                lcd_command(0x0F);      // not the correct cursor blink but it is blinking
                                break;
                        }
                        break;

                    case 3: // RTC MENU
                        // example: modify RTC field
                        switch (selected_index)
                        {
                            case 1: rtc_year++; break;
                            case 2: rtc_month++; break;
                            case 3: rtc_day++; break;
                            case 4: rtc_hour++; break;
                            case 5: rtc_minute++; break;
                            case 6: rtc_second++; break;
                            default:
                                current_screen = 1; // back to main
                                break;
                        }
                        break;

                    case 2: // COLOR MENU
                        // handle RGB selection
                        switch (selected_index) {
                            case 1: 
                                break;
                            case 2: 
                                break;
                            case 3: 
                                break;
                            default: 
                                current_screen = 0;
                                break;
                        }
                        break;

                    case 0: // HOME SCREEN
                        current_screen = 1;
                        break;
                }
                break;

            default:
                break;
        }
   
        spi_write(LCD_CONT);
        __delay_cycles(10);
        read_temp_sensor();
        rtc_update_time();


        switch(current_screen) {
            case 0: 
                write_home_screen();
                break;
            case 1: 
                write_main_menu();
                break;
            case 2: 
                write_color_menu();
                break;
            case 3: 
                write_rtc_menu();
                break;
            default: 
                write_main_menu();
                break;
        }

        if (led_step_flag)
         {
            led_step_flag = 0;
            led_bar_step();
         }

        //current_screen++;
        //current_first_row++;
        __delay_cycles(2000);
    }
   
    return 0;
}
