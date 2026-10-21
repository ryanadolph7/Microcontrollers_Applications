#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

// Initialize UART (8N1). Uses SMCLK as clock source.
void uart_init(uint32_t smclk_hz, uint32_t baud);

// Non-blocking RX helpers
bool uart_available(void);     // any bytes in RX buffer?
int  uart_getc(void);          // returns 0..255, or -1 if none

// Line-oriented RX (returns true when a full line is ready)
bool uart_line_available(void);
uint16_t uart_readline(char *dst, uint16_t max_len); // returns length (0 if none)

// TX helpers (blocking TX)
void uart_putc(char c);
void uart_puts(const char *s);

#endif
