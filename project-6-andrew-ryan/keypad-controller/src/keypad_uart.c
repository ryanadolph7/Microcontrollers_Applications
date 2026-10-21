#include <msp430fr2310.h>
#include <stdbool.h>
#include <stdint.h>

#define TX              BIT7        // P1.6 and 1.7 used for TX / RX over uart
#define RX              BIT6

void uart_init(void) {
        // hold in reset  use SMCLK
    UCA0CTLW0 = UCSWRST | UCSSEL__SMCLK;
    UCA0BRW = 8;            // 115200 baud rate
    UCA0MCTLW = 0xD600;
    P1SEL1 &= ~(TX | RX);
    P1SEL0 |= (TX | RX);
    UCA0CTLW0 &= ~UCSWRST;      // ucswrst = 1 for eusci_b0 in sw reset
    return;
}

// helper functions to send characters and strings
void uart_send_char(char c) {
    while(!(UCA0IFG & UCTXIFG)); // wait for tx buffer ready 
    UCA0TXBUF = c;
}

void uart_send_string(const char *str) {
    while (*str) {
        uart_send_char(*str++);
    }
}