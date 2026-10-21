#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>


void delay_timer_ms(unsigned int ms);
void lcd_pulse_enable(void);
void lcd_send_nibble(unsigned char nibble);
void lcd_send_byte(unsigned char byte, unsigned char rs);
void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_data_init(void);
void lcd_init(void);
void timer_init(void);
void spi_init(void);
void spi_write(uint16_t word);
void write_all(void);
