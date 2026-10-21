#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "uart.h"

#define UART_RX_BUF_SZ 128

static volatile uint8_t  g_rx_buf[UART_RX_BUF_SZ];
static volatile uint16_t g_rx_w = 0;
static volatile uint16_t g_rx_r = 0;
static volatile bool     g_line_ready = false;

static void uart_pins_to_uart_mode(void)
{
    // Set TX/RX pins to UART function (typical: SEL0=1, SEL1=0).
    // If your FR2153 pin mux differs, adjust these in board.h.
    UART_PORT_SEL0 |= (UART_TX_BIT | UART_RX_BIT);
    UART_PORT_SEL1 &= (uint8_t)~(UART_TX_BIT | UART_RX_BIT);
}

void uart_init(uint32_t smclk_hz, uint32_t baud)
{
    uint16_t brw;

    uart_pins_to_uart_mode();

    // Hold eUSCI in reset
    UCA0CTLW0 = UCSWRST;

    // SMCLK, 8-bit, no parity, 1 stop, UART mode
    UCA0CTLW0 |= UCSSEL__SMCLK;

    if (baud == 0) baud = 9600UL;
    brw = (uint16_t)(smclk_hz / baud);
    if (brw < 1) brw = 1;

    UCA0BRW = brw;
    UCA0MCTLW = 0; // simple mode (no oversampling config)

    // Clear flags, enable RX interrupt
    UCA0IFG = 0;
    UCA0IE  = UCRXIE;

    // Release reset
    UCA0CTLW0 &= (uint16_t)~UCSWRST;

    // reset buffer state
    g_rx_w = g_rx_r = 0;
    g_line_ready = false;
}

bool uart_available(void)
{
    return g_rx_w != g_rx_r;
}

int uart_getc(void)
{
    uint8_t c;

    if (g_rx_w == g_rx_r) return -1;

    c = g_rx_buf[g_rx_r];
    g_rx_r = (uint16_t)((g_rx_r + 1u) % UART_RX_BUF_SZ);
    return (int)c;
}

bool uart_line_available(void)
{
    return g_line_ready;
}

uint16_t uart_readline(char *dst, uint16_t max_len)
{
    uint16_t n = 0;
    int ch;

    if (!g_line_ready || max_len == 0) return 0;

    // read until '\n' (consume '\r' too)
    while ((ch = uart_getc()) != -1) {
        char c = (char)ch;

        if (c == '\r') continue;

        if (c == '\n') {
            break;
        }

        if (n + 1u < max_len) {
            dst[n++] = c;
        }
    }

    dst[n] = '\0';
    g_line_ready = false; // consumed one full line
    return n;
}

void uart_putc(char c)
{
    while ((UCA0IFG & UCTXIFG) == 0) { }
    UCA0TXBUF = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

// RX ISR
#pragma vector = EUSCI_A0_VECTOR
__interrupt void EUSCI_A0_ISR(void)
{
    if (UCA0IFG & UCRXIFG) {
        uint8_t c = UCA0RXBUF;

        g_rx_buf[g_rx_w] = c;
        g_rx_w = (uint16_t)((g_rx_w + 1u) % UART_RX_BUF_SZ);

        // crude overflow handling: drop oldest
        if (g_rx_w == g_rx_r) {
            g_rx_r = (uint16_t)((g_rx_r + 1u) % UART_RX_BUF_SZ);
        }

        if (c == '\n') {
            g_line_ready = true;
        }
    }
}
