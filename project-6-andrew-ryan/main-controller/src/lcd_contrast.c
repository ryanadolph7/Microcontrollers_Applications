#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "lcd_contrast.h"
#include "temp_sensor.h"
#include "rtc.h"


// Define bit masks for LED and LCD pins
#define LED         BIT0          // Heartbeat LED on P2.0


// Use LCD data lines on P1.4, P1.5, P1.6, and P1.7
#define LCD_D4      BIT4
#define LCD_D5      BIT5
#define LCD_D6      BIT6
#define LCD_D7      BIT7
#define LCD_DATA    (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)


// LCD control pins on P4.0 (RS) and 4.1 (Enable)
#define LCD_RS      BIT0          // P4.0
#define LCD_E       BIT1          // P4.1


// DAC control pins for SPI on 4.4 (CS - Chip Select), 4.5 (CLK), 4.6 (SIMO)
#define CS          BIT4
#define CLK         BIT5
#define DTA         BIT6

#define DAC_DATA    (CS | CLK | DTA)
volatile uint16_t LCD_CONT = 1;
int line = 0;
int data_sent = 0;

// RTC screen data 
char *rtc_menu[] = {"Main screen", "Year", "Month", "Date", "Hour", "Minute", "Second"};

// Home screen data
extern volatile float temp; 
extern volatile uint8_t pattern;
volatile float period_count = 0.1f;
volatile char *current_temp;
volatile uint8_t current_temp_whole = 20;
volatile uint8_t current_temp_decimal = 5;
volatile uint8_t current_window_size = 05;
char *curr_pattern[] = {"toggle", "up counter", "in and out", "down counter", "rotate left", "fill right"};
char *period[] = {"Period = "};
char *temperature[] = {" "};
char *rtc[] = {" "};

// main menu data
char *main_menu[] = {"Main screen", "Window size", "LCD contrast", "RTC settings", "WS2812B color", "Cursor", "Blink"};
volatile int current_first_row = 0; // acting as scroll
volatile int CUR_WINDOW_SIZE = 5;
extern volatile int CURSOR_ON = 1;
extern volatile int BLINK_ON = 0;
extern volatile int current_row = 0;

// line commands to send cursor to specified line
unsigned char LINE_1 = 0x80;
unsigned char LINE_2 = 0xC0;
unsigned char LINE_3 = 0x94;
unsigned char LINE_4 = 0xD4;
unsigned char all_lines[] = {0x80, 0xC0, 0x94, 0xD4};
unsigned char CLEAR_SCREEN = 0x01;


#define LINE_CNT    20
// packets for SPI to DAC
unsigned int position;      // for indexing ito dac_packet
uint16_t dac_packet[] = {0b0011000011110000, 0b0011000011110000};
//          0 - ~GA ~SHDN D7 D6 D5 D4, D3 D2 D1 D0 - - - - 

//-------------------------------------------------
// Timer-Based Delay Function using Timer_B
//-------------------------------------------------
// This function uses Timer_B's counter (TB0R), which runs on ACLK (32768Hz).
// It calculates the number of counts corresponding to the requested delay (in ms)
// and polls TB0R until that many counts have elapsed (handling counter wrap-around).
void delay_timer_ms(unsigned int ms) {
    // For ACLK = 32768 Hz, counts = ms * 32768 / 1000.
    unsigned int counts = (ms * 32768UL) / 1000;
    unsigned int start = TB0R;
    unsigned int elapsed;


    while (1) {
        unsigned int current = TB0R;
        if (current >= start)
            elapsed = current - start;
        else
            // Handle wrap-around: TB0CCR0 is 16383 (period = 16384 counts)
            elapsed = (TB0CCR0 - start) + current + 1;
       
        if (elapsed >= counts)
            break;
    }
}

//-------------------------------------------------
// LCD Low-Level Functions
//-------------------------------------------------


// Pulse the Enable pin to latch data into the LCD.
void lcd_pulse_enable(void) {
    P4OUT |= LCD_E;         // Set Enable high on P2.7
    __delay_cycles(25);       // Short fixed delay for edge timing
    P4OUT &= ~LCD_E;        // Set Enable low
    __delay_cycles(25);
}


// Send a 4-bit nibble to the LCD.
// This maps the lower nibble to P1.4–P1.7 (bit0 → P1.4, bit1 → P1.5, etc.).
void lcd_send_nibble(unsigned char nibble) {
    P1OUT &= ~LCD_DATA;                     // Clear LCD data pins (P1.4–P1.7)
    P1OUT |= ((nibble & 0x0F) << 4);          // Shift nibble so that bit0 maps to P1.4
    lcd_pulse_enable();
}


// Send a full byte (split into two nibbles) to the LCD with the given RS value.
// rs = 0 for command; rs = 1 for data.
void lcd_send_byte(unsigned char byte, unsigned char rs) {
    if(rs)
        P4OUT |= LCD_RS;   // Set RS high for data on P2.6
    else
        P4OUT &= ~LCD_RS;  // Clear RS for command
   
    // Send the high nibble first, then the low nibble.
    lcd_send_nibble(byte >> 4);
    lcd_send_nibble(byte & 0x0F);
    __delay_cycles(100);
    //delay_timer_ms(2); // Delay to allow LCD processing
}


// Convenience functions for commands and data writes.
void lcd_command(unsigned char cmd) {
    lcd_send_byte(cmd, 0);
}


void lcd_data(unsigned char data) {
    lcd_send_byte(data, 1);
}


//-------------------------------------------------
// LCD Initialization Sequence for 4-Bit Mode
//-------------------------------------------------

void lcd_data_init(void) {
      
    // Configure LCD data pins (P1.4–P1.7) as outputs.
    P1DIR |= LCD_DATA;
    P1OUT &= ~LCD_DATA;
   
    // Configure LCD control pins (4.0 and P4.1) as outputs.
    P4DIR |= (LCD_RS | LCD_E);
    P4OUT &= ~(LCD_RS | LCD_E);
}


void lcd_init(void) {
    //delay_timer_ms(20); // Wait >15ms after power-up
    __delay_cycles(1000);


    // Start in 8-bit mode by sending high nibble 0x03 three times.
    P4OUT &= ~LCD_RS;
    lcd_send_nibble(0x03);
    //delay_timer_ms(5);
    __delay_cycles(500);
    lcd_send_nibble(0x03);
    //delay_timer_ms(5);
    __delay_cycles(500);
    lcd_send_nibble(0x03);
    //delay_timer_ms(1);
    __delay_cycles(500);
   
    // Switch to 4-bit mode.
    lcd_send_nibble(0x02);
    //delay_timer_ms(1);
    __delay_cycles(500);
   
    // Send function set: 4-bit mode, 2-line display, 5x8 font.
    lcd_command(0x28);
    // Display ON, cursor OFF, blink OFF.
    lcd_command(0x0C);
    // Clear display.
    lcd_command(0x01);
    //delay_timer_ms(1);
    __delay_cycles(500);
    // Entry mode set: Increment cursor, no display shift.
    lcd_command(0x06);
}


//-------------------------------------------------
// Timer Initialization for Heartbeat LED & Delay
//-------------------------------------------------
// Configure Timer_B (TB0) in up mode using ACLK (32768Hz).
// TB0CCR0 is set to 16383 to create a period of ~500ms.
void timer_init(void) {
    TB0CCR0 = 16384 - 1;                      // Period = 16384 counts (~500ms)
    //TB0CCTL0 = CCIE;                          // Enable CCR0 interrupt
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;     // Use ACLK, up mode, clear timer
}

// ---------------------------------------------
// SPI initialization for DAC communication
// ---------------------------------------------
void spi_init(void) {
    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL__SMCLK;     // choose SMCLK = 1MHz and divide 10 to give SCLK = 100 kHz
    UCB1BRW = 10;

    UCB1CTLW0 |= UCSYNC;            // Put eUSCI_A0 into SPI mode (3 pin)
    UCB1CTLW0 |= UCMST;             // SPI master

    P4SEL1 &= ~DAC_DATA;            // config pins for SPI
    P4SEL0 &= ~DAC_DATA;             // define as constants 
    P4DIR |= DAC_DATA;
    P4OUT &= ~CLK;
    P4OUT &= ~DTA;
    P4OUT |= CS;
}

void spi_write(uint16_t word) {
    // command bits for the DAC (GA, SHDN) | our value we want 
    uint16_t value = 0x3000 | word << 4; // got this shift 4 thing from chatGPT cause my outputs were being weird when I tried to do it without it

    int i;
    P4OUT &= ~CS;
    //delay_timer_ms(1);
    for (i = 0; i < 16; i++) {
        if (value & 0x8000) {
            P4OUT |= DTA;
        } else {
            P4OUT &= ~DTA;
        }
        //delay_timer_ms(.5);
        __delay_cycles(10);
        P4OUT |= CLK;
        //delay_timer_ms(.5);
        __delay_cycles(10);
        P4OUT &= ~CLK;
        //delay_timer_ms(.5);
        __delay_cycles(10);
        value <<= 1;
    }
    P4OUT |= CS;
    //delay_timer_ms(.5);
}


/**
 * was using snprintf() to concatenate strings but i ran into a memory issue so i asked chat gpt for a little help
 * and he squirted out this hot garbage but it works
 */
void append_string(char *buf, char *string) {
    int i = 0;
    while (buf[i] != '\0') {
        i++;
    }
    int j = 0;
    while (string[j] != '\0') {
        buf[i + j] = string[j];
        j++;
    }
    buf[i + j] = '\0';
    return;
}

void append_char(char *buf, char c) {
    int i = 0;
    while (buf[i] != '\0') {
        i++;
    }
    buf[i] = c;
    buf[i + 1] = '\0';
    return;
}

void append_int(char *buf, unsigned int value) {
    char temp[6];
    int i= 0;
    int j = 0;
    if (value == 0) {
        append_char(buf, '0');
        return;
    }
    while(value > 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }
    for (j = i - 1; j >= 0; j--) {
        append_char(buf, temp[j]);
    }

    return;
}

/**
 * Helper function that writes to each line of the LCD screen 
 * need to give it the line you want to print to and the actual string you want to print
 */

void write_line(unsigned char command, char *buffer) {
    lcd_command(command);       // send the cursor to the desired line
    int i; 
    for (i = 0; i < LINE_CNT; i++) {
        if (buffer[i] == '\0') {
            break;
        } else {
            lcd_data(buffer[i]);
        }
    }
    return;
}

void write_rtc_menu(void) {
    //lcd_command(CLEAR_SCREEN);
    //current_first_row = 0;
    int i;
    for (i = 0; i < 4; i++) {
        unsigned int rtc_menu_idx = i + current_first_row;
        char buffer[20];
        if (i == current_row && CURSOR_ON == 1) {
            buffer[0] = '>';
        } else {
            buffer[0] = ' ';
        }
        if (strcmp(rtc_menu[rtc_menu_idx], "Year") == 0) {
            buffer[1] = '\0';
            append_string(buffer, "Year           ");
            append_int(buffer, rtc_year);
            write_line(all_lines[i], buffer);
        } else if (strcmp(rtc_menu[rtc_menu_idx], "Month") == 0) {
            buffer[1] = '\0';
            append_string(buffer, "Month             ");
            append_int(buffer, rtc_month);
            write_line(all_lines[i], buffer);
        } else if (strcmp(rtc_menu[rtc_menu_idx], "Date") == 0) {
            buffer[1] = '\0';
            append_string(buffer, "Date             ");
            append_int(buffer, rtc_day);
            write_line(all_lines[i], buffer);
        } else if (strcmp(rtc_menu[rtc_menu_idx], "Hour") == 0) {
            buffer[1] = '\0';
            append_string(buffer, "Hour             ");
            append_int(buffer, rtc_hour);
            write_line(all_lines[i], buffer);
        } else if (strcmp(rtc_menu[rtc_menu_idx], "Minute") == 0) {
            buffer[1] = '\0';
            append_string(buffer, "Minute           ");
            append_int(buffer, rtc_minute);
            write_line(all_lines[i], buffer);
        } else if (strcmp(rtc_menu[rtc_menu_idx], "Second") == 0) {
            buffer[1] = '\0';
            append_string(buffer, "Second           ");
            append_int(buffer, rtc_second);
            write_line(all_lines[i], buffer);
        } else {
            write_line(all_lines[i], "Main menu");
        }

    }
    return;
}



/**
 * Writes the whole color screen to the LCD display
 * - need to implement the Entry + displaying the actual value being sent to the color bar 
 *      for our Red + Green + Blue values 
 */
void write_color_menu(void) {                  
    //lcd_command(CLEAR_SCREEN);
    __delay_cycles(10);
    if (current_row == 0) {
        write_line(all_lines[0], ">Main menu");
        write_line(all_lines[1], " Red");
        write_line(all_lines[2], " Green");
        write_line(all_lines[3], " Blue");
    } else if (current_row == 1) {
        write_line(all_lines[0], " Main menu");
        write_line(all_lines[1], ">Red");
        write_line(all_lines[2], " Green");
        write_line(all_lines[3], " Blue");
    } else if (current_row == 2) {
        write_line(all_lines[0], " Main menu");
        write_line(all_lines[1], " Red");
        write_line(all_lines[2], ">Green");
        write_line(all_lines[3], " Blue");
    } else if (current_row == 3) {
        write_line(all_lines[0], " Main menu");
        write_line(all_lines[1], " Red");
        write_line(all_lines[2], " Green");
        write_line(all_lines[3], ">Blue");
    }
    
    //__delay_cycles(25);
    return;
}

/** 
 * Writes the whole home screen to the LCD display
 * - need to implement the RTC values being updated and the temp sensor values to display 
 * - need to add the current temperature to the screen as well
 */
void write_home_screen(void) {  
    //lcd_command(CLEAR_SCREEN);      // clear the screen
    read_temp_sensor();
    append_char(curr_pattern[pattern], 0x11101111);
    write_line(all_lines[0], curr_pattern[pattern]);
    char period_buff[20];
    period_buff[0] = '\0';
    append_string(period_buff, "Period = ");
    append_int(period_buff, period_count);
    append_string(period_buff, " s");
    write_line(all_lines[1], period_buff);
    char temp_buff[20];
    temp_buff[0] = '\0';
    append_string(temp_buff, (char *)AVG_TEMP);
    append_string(temp_buff, " C");
    append_string(temp_buff, "      N = ");
    append_int(temp_buff, average_window_size);
    write_line(all_lines[2], temp_buff);
    char rtc_buff[20];
    rtc_buff[0] = '\0';
    append_int(rtc_buff, rtc_month);
    append_string(rtc_buff, "/");
    append_int(rtc_buff, rtc_day);
    append_string(rtc_buff, "/");
    append_int(rtc_buff, rtc_year);
    append_string(rtc_buff, "  ");
    if (rtc_minute < 10){
        append_string(rtc_buff, "0");
    }
    append_int(rtc_buff, rtc_hour);
    append_string(rtc_buff, ":");
    if (rtc_minute < 10){
        append_string(rtc_buff, "0");
    }
    append_int(rtc_buff, rtc_minute);
    append_string(rtc_buff, ":");
    if (rtc_second < 10){
        append_string(rtc_buff, "0");
    }
    append_int(rtc_buff, rtc_second);
    write_line(all_lines[3], rtc_buff);
    //__delay_cycles(25);
    return;
}


/**
 * Function that writes + updates the main menu when scrolling through the menu
 * - updates what we are showing for the LCD contrast 
 * - updates what we are showing for the current temp sensor avg window size
 * - need to implement entry function to turn on / off the blinking cursor + cursor on / off 
 */
void write_main_menu(void) {
    //lcd_command(CLEAR_SCREEN);
    unsigned int i;
    for (i = 0; i < 4; i++) {
        unsigned int menu_idx = i + current_first_row;
        char buffer[20];
        if (i == current_row && CURSOR_ON == 1) {
            buffer[0] = '>';
        } else {
            buffer[0] = ' ';
        }
        if (strcmp(main_menu[menu_idx], "LCD contrast") == 0 ) {            // update the current LCD contrast print line 
            buffer[1] = '\0';
            append_string(buffer, "LCD contrast     ");
            append_int(buffer, LCD_CONT);
            write_line(all_lines[i], buffer);
        } else if (strcmp(main_menu[menu_idx], "Window size") == 0) {       // update the current window size print line
            buffer[1] = '\0';
            append_string(buffer, "Window size      ");
            append_int(buffer, CUR_WINDOW_SIZE);
            write_line(all_lines[i], buffer);
        } else if (strcmp(main_menu[menu_idx], "Cursor") == 0) {            // update if cursor is on / off
            buffer[1] = '\0';
            append_string(buffer, "Cursor    ");
            if (CURSOR_ON == 1) {           // we want to the cursor on
                append_string(buffer, "[on] off");
                write_line(all_lines[i], buffer);
            } else {
                append_string(buffer, "on [off]");
                write_line(all_lines[i], buffer);
            }
        } else if (strcmp(main_menu[menu_idx], "Blink") == 0) {             // update if we want the cursor to blink / not 
            buffer[1] = '\0';
            append_string(buffer, "Blink     ");
            if (BLINK_ON == 1) {           // we want to the blinking cursor on
                append_string(buffer, "[on] off");
                write_line(all_lines[i], buffer);
            } else {
                append_string(buffer, "on [off]");
                write_line(all_lines[i], buffer);
            }
        } else {
            buffer[1] = '\0';
            append_string(buffer, main_menu[menu_idx]);
            write_line(all_lines[i], buffer);
        }  
    }
    return;
}
