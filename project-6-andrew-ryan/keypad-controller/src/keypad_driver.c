#include <msp430fr2310.h>
#include <stdbool.h>
#include <stdint.h>


#define KEY_R1          BIT1        // PIN 1 for keypad row outputs
#define KEY_R2          BIT3        // these are used to drive the keypad 
#define KEY_R3          BIT4        // to receive outputs from the columns
#define KEY_R4          BIT5
#define KEY_ROWS        (KEY_R1 | KEY_R2 | KEY_R3 | KEY_R4)
const uint8_t keys[] = {KEY_R1, KEY_R2, KEY_R3, KEY_R4};
int num_rows = 4;           // used for cycling rows 
int row_index = 0;

#define KEY_C1          BIT0        // PIN 2 for keypad column inputs 
#define KEY_C2          BIT6  
#define KEY_C3          BIT7
#define KEY_COLS        (KEY_C1 | KEY_C2 | KEY_C3)
uint8_t col_output = 0;

#define COL1            {'1', '4', '7', '*'}
#define COL2            {'2', '5', '8', '0'}
#define COL3            {'3', '6', '9', '#'}
volatile char current_character;
volatile int col_1_press;
volatile int col_2_press;
volatile int col_3_press;

void keypad_init(void) {
    P1DIR |= KEY_ROWS;      // setup rows as output pins
    P1OUT &= ~KEY_ROWS;     // clear the outputs

    P2DIR &= ~KEY_COLS;     // setup cols as input pins
    P2REN |= KEY_COLS;
    P2OUT &= ~KEY_COLS;     // enable pull down resistors
    P2IES &= ~KEY_COLS;     // enable L-to-H interrupt on keypad column pins
    P2IFG &= ~KEY_COLS;     // clear P2.0, 2.6, 2.7 IRQ flag
    P2IE |= KEY_COLS;       // enable maskable IRQ

    return;
}

// shifts each of our rows high for our keypad to multiplex the column outputs
// needs a small delay called in main function - should put that here instead so this file holds the delay + interrupt

void shift_high_row(void) {
    if (row_index >= num_rows) {
        row_index = 0;
    }
    P1OUT &= ~KEY_ROWS;
    P1OUT |= keys[row_index];
    row_index++;
    return;
}

// gets the current key being pressed in each column on the keypad based off of the row index1
// -- sets the current character that should be sent over uart 
//      -- current_character = colX_char[row_index - 1]
#pragma vector=PORT2_VECTOR
__interrupt void ISR_PORT2_KEYPAD(void) {
    int i;
    char col1_char[] = COL1;
    char col2_char[] = COL2;
    char col3_char[] = COL3;

    col_output = P2IN & KEY_COLS;      // -- binary num from P2IN
    switch (col_output) {
        case 1:             // column 1 output 
            current_character = col1_char[row_index - 1];
            //is_pressed = 1;
            col_1_press++;
            break;
        case 64:            // column 2 output
            current_character = col2_char[row_index - 1];
            //is_pressed = 1;
            col_2_press++;
            break;
        case 128:           // column 3 output
            current_character = col3_char[row_index - 1];  
            //is_pressed = 1;
            col_3_press++;
            break;
        default: 
            //is_pressed = 0;
            col_1_press = 0;
            col_2_press = 0;
            col_3_press = 0;
            break;
    }
    
    P2IFG &= ~KEY_COLS;
    return;
}