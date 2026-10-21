#ifndef LCD_CONTRAST_H
#define LCD_CONTRAST_H

#include <stdint.h>

extern unsigned char LINE_1;
extern unsigned char LINE_2;
extern unsigned char LINE_3;
extern unsigned char LINE_4;
extern volatile int current_first_row;
extern volatile uint16_t LCD_CONT;
extern volatile int curr_pattern_index_idx;

void delay_timer_ms(unsigned int ms);
void lcd_pulse_enable(void);
void lcd_send_nibble(unsigned char nibble);
void lcd_send_byte(unsigned char byte, unsigned char rs);
void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_data_init(void);
void lcd_init(void);
void timer_init(void);
//void spi_init(void);
//void spi_write(uint16_t word);
void write_line(unsigned char command, char *buffer); 
void append_int(char *buf, unsigned int value);
void append_char(char *buf, char c);
void append_string(char *buf, char *string);

#endif
