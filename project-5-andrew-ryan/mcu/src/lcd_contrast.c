#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "lcd_contrast.h"

// Define bit masks for LED and LCD pins
#define LED         BIT0          // Heartbeat LED on P2.0


// Use LCD data lines on P1.4, P1.5, P1.6, and P1.7
#define LCD_D4      BIT4
#define LCD_D5      BIT5
#define LCD_D6      BIT6
#define LCD_D7      BIT7
#define LCD_DATA    (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)


// LCD control pins on P2.6 (RS) and P2.7 (Enable)
#define LCD_RS      BIT6          // P2.6
#define LCD_E       BIT7          // P2.7


// DAC control pins for SPI on 4.4 (CS - Chip Select), 4.5 (CLK), 4.6 (SIMO)
#define CS          BIT4
#define CLK         BIT5
#define DTA         BIT6

#define DAC_DATA    (CS | CLK | DTA)
int line = 0;
int data_sent = 0;
// packets for LCD screen
char *curr_pattern[] = {"toggle", "up counter", "in and out", "down counter", "rotate left", "fill right"};
char *period[] = {"Period = ", " " , " s"};
char *temp[] = {"Temp = ", 0b11011111 ," C"};
char *rtc[] = {" ", " "};

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
    P2OUT |= LCD_E;         // Set Enable high on P2.7
    __delay_cycles(50);       // Short fixed delay for edge timing
    P2OUT &= ~LCD_E;        // Set Enable low
    __delay_cycles(50);
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
        P2OUT |= LCD_RS;   // Set RS high for data on P2.6
    else
        P2OUT &= ~LCD_RS;  // Clear RS for command
   
    // Send the high nibble first, then the low nibble.
    lcd_send_nibble(byte >> 4);
    lcd_send_nibble(byte & 0x0F);
    delay_timer_ms(2); // Delay to allow LCD processing
}


// Convenience functions for commands and data writes.
void lcd_command(unsigned char cmd) {
    lcd_send_byte(cmd, 0);
}


void lcd_data(unsigned char data) {
    lcd_send_byte(data, 1);
    data_sent++;
    if (data_sent % 20 == 0) {
        line ++;
    }
    if (line >= 5) {
        line = 0;
    }
}

//-------------------------------------------------
// LCD Initialization Sequence for 4-Bit Mode
//-------------------------------------------------

void lcd_data_init(void) {
      
    // Configure LCD data pins (P1.4–P1.7) as outputs.
    P1DIR |= LCD_DATA;
    P1OUT &= ~LCD_DATA;
   
    // Configure LCD control pins (P2.6 and P2.7) as outputs.
    P2DIR |= (LCD_RS | LCD_E);
    P2OUT &= ~(LCD_RS | LCD_E);
}


void lcd_init(void) {
    delay_timer_ms(20); // Wait >15ms after power-up


    // Start in 8-bit mode by sending high nibble 0x03 three times.
    P2OUT &= ~LCD_RS;
    lcd_send_nibble(0x03);
    delay_timer_ms(5);
    lcd_send_nibble(0x03);
    delay_timer_ms(5);
    lcd_send_nibble(0x03);
    delay_timer_ms(1);
   
    // Switch to 4-bit mode.
    lcd_send_nibble(0x02);
    delay_timer_ms(1);
   
    // Send function set: 4-bit mode, 2-line display, 5x8 font.
    lcd_command(0x28);
    // Display ON, cursor OFF, blink OFF.
    lcd_command(0x0C);
    // Clear display.
    lcd_command(0x01);
    delay_timer_ms(2);
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
    TB0CCTL0 = CCIE;                          // Enable CCR0 interrupt
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
        P4OUT |= CLK;
        //delay_timer_ms(.5);
        P4OUT &= ~CLK;
        //delay_timer_ms(.5);
        value <<= 1;
    }
    P4OUT |= CS;
    //delay_timer_ms(.5);
}

void write_all(void) {
    if (line >= 4) {
        line = 0;
    }
    int i;
    char *current_line;
    switch (line) {
        case 0: 
            current_line = curr_pattern[0];
            for (i = 0; i < 20; i++) {
                if (current_line[i] == '\0') {
                    lcd_data(' ');
                    line++;
                    lcd_command(0x94);  // send cursor to next line
                    break;
                } else {
                    lcd_data(current_line[i]);
                }
            }
            break;
        case 1: 
            current_line = temp[0];
            for (i = 0; i < 20; i++) {
                if (current_line[i] == '\0') {
                    lcd_data(' ');
                    line++;
                    lcd_command(0xC0);      // send cursor to next line
                    break;
                } else {
                    lcd_data(current_line[i]);
                }
            }
            break;
        case 2: 
            current_line = period[0];
            for (i = 0; i < 20; i++) {
                if (current_line[i] == '\0') {
                    lcd_data(' ');
                    line++;
                    lcd_command(0xD4);      // send cursor to next line
                    break;
                } else {
                    lcd_data(current_line[i]);
                }
            }
            break;
        case 3: 
           current_line = rtc[0];
            for (i = 0; i < 20; i++) {
                if (current_line[i] == '\0') {
                    lcd_data(' ');
                    line++;
                    lcd_command(0x80);      // send cursor to next line
                    break;
                } else {
                    lcd_data(current_line[i]);
                }
            }
            break;
        default: 
            break;
    }
}
