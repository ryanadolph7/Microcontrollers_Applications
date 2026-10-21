#ifndef KEYPAD_DRIVER_H
#define KEYPAD_DRIVER_H

extern volatile char current_character;
//extern volatile int is_pressed;
extern volatile int col_1_press;
extern volatile int col_2_press;
extern volatile int col_3_press;
void keypad_init(void);
void shift_high_row(void);

#endif
