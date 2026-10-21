#ifndef UART_DRIVER_H
#define UART_DRIVER_H

// Using Port 4.2, 4.3 for uart to the MSP430FR2310
extern volatile char uart_rx_buffer[32];
extern volatile uint8_t uart_rx_index;
extern volatile bool uart_line_ready;


void uart_init(void);
void uart_send_char(char c);
void uart_send_string(char *str);


#endif