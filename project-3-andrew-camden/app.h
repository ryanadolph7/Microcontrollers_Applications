#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

void app_init(void);

// Called from TimerB0 ISR every 1ms; returns true if ISR should exit LPM0
bool app_isr_tick_1ms(void);

// Called from Port3 ISR; returns true if ISR should exit LPM0
bool app_isr_port3(void);

// Called from main loop when awakened
void app_task(void);

#endif
