#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#define UART_TX     BIT3
#define UART_RX     BIT2

volatile char uart_rx_buffer[32];
volatile uint8_t uart_rx_index = 0;
volatile bool uart_line_ready = false;

/**
 * UART init that uses Port 4.2 and 4.3 for UART TX and RX
 * - buad rate is 115200 to match other UART transmissions 
 */
void uart_init(void) {
    UCA1CTLW0 |= UCSWRST;
    UCA1CTLW0 |= UCSSEL__SMCLK;
    UCA1BRW = 8;            
    UCA1MCTLW = 0xD600; 
    P4DIR |= UART_TX;
    P4DIR &= ~UART_RX;
    P4SEL1 &= ~(UART_RX | UART_TX);
    P4SEL0 |= (UART_RX | UART_TX);
    UCA1IFG &= ~UCRXIFG;
    UCA1CTLW0 &= ~UCSWRST;
    UCA1IE |= UCRXIE;
}

/**
* Helper functions to send characters and strings via UART
*/
void uart_send_char(char c) {
    while(!(UCA1IFG & UCTXIFG)); // wait for tx buffer ready 
    UCA1TXBUF = c;
}

void uart_send_string(const char *str) {
    while (*str) {
        uart_send_char(*str++);
    }
}

#pragma vector=EUSCI_A1_VECTOR
__interrupt void EUSCI_A1_ISR(void)
{
    if (UCA1IFG & UCRXIFG) {
        char c = UCA1RXBUF;   // this reads/intercepts the incoming byte

        if (!uart_line_ready) {
            if (c == '\r' || c == '\n') {
                uart_rx_buffer[uart_rx_index] = '\0';
                uart_rx_index = 0;
                uart_line_ready = true;
            } else {
                if (uart_rx_index < sizeof(uart_rx_buffer) - 1) {
                    uart_rx_buffer[uart_rx_index++] = c;
                } else {
                    // buffer full: terminate and mark ready
                    uart_rx_buffer[uart_rx_index] = '\0';
                    uart_rx_index = 0;
                    uart_line_ready = true;
                }
            }
        }
    }
}