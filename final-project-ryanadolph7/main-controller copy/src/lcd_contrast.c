#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "lcd_contrast.h"


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
volatile uint16_t LCD_CONT = 2;
int line = 0;
int data_sent = 0;


// line commands to send cursor to specified line
unsigned char LINE_1 = 0x80;
unsigned char LINE_2 = 0xC0;
unsigned char LINE_3 = 0x94;
unsigned char LINE_4 = 0xD4;
unsigned char all_lines[] = {0x80, 0xC0, 0x94, 0xD4};
unsigned char CLEAR_SCREEN = 0x01;

#define LINE_CNT    19
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
    delay_timer_ms(20); // Wait >15ms after power-up


    // Start in 8-bit mode by sending high nibble 0x03 three times.
    P4OUT &= ~LCD_RS;
    lcd_send_nibble(0x03);
    delay_timer_ms(5);
    //__delay_cycles(500);
    lcd_send_nibble(0x03);
    delay_timer_ms(5);
    //__delay_cycles(500);
    lcd_send_nibble(0x03);
    delay_timer_ms(1);
    //__delay_cycles(500);
   
    // Switch to 4-bit mode.
    lcd_send_nibble(0x02);
    delay_timer_ms(1);
    //__delay_cycles(500);
   
    // Send function set: 4-bit mode, 2-line display, 5x8 font.
    lcd_command(0x28);
    delay_timer_ms(1);
    // Display ON, cursor OFF, blink OFF.
    lcd_command(0x0C);
    delay_timer_ms(1);
    // Clear display.
    lcd_command(0x01);
    delay_timer_ms(1);
    delay_timer_ms(1);
    // Entry mode set: Increment cursor, no display shift.
    lcd_command(0x06);
    delay_timer_ms(1);
}


//-------------------------------------------------
// Timer Initialization for Heartbeat LED & Delay
//-------------------------------------------------
// Configure Timer_B (TB0) in up mode using ACLK (32768Hz).
// TB0CCR0 is set to 16383 to create a period of ~500ms.
void timer_init(void) {
    TB0CCR0 = 16384 - 1;                      // Period = 16384 counts (~500ms)
    TB0CCTL0 = 0;                          // Enable CCR0 interrupt
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;     // Use ACLK, up mode, clear timer
}


/**
 * was using snprintf() to concatenate strings but i ran into a memory issue so i asked chat gpt for a little help
 * and he squirted out this hot garbage but it works
 */
 #define MAX_LEN 20
void append_string(char *buf, char *string) {
    int i = 0;
    while (buf[i] != '\0' && i < MAX_LEN - 1) {
        i++;
    }
    int j = 0;
    while (string[j] != '\0' && i < MAX_LEN - 1) {
        buf[i++] = string[j];
        j++;
    }
    buf[i] = '\0';
    return;
}

void append_char(char *buf, char c) {
    int i = 0;
    while (buf[i] != '\0') {
        i++;
    }
    if (i < MAX_LEN) {
        buf[i] = c;
        buf[i + 1] = '\0';
    }
    
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
    //lcd_data(' ');
    for (i = 0; i < LINE_CNT; i++) {
        if (buffer[i] == '\0') {
            break;
        } else {
            lcd_data(buffer[i]);
        }
    }
    return;
}




