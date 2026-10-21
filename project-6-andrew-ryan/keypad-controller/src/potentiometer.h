

#ifndef SRC_POTENTIOMETER_H_
#define SRC_POTENTIOMETER_H_

#include <stdint.h>

extern volatile unsigned int ADC_VALUE;

extern volatile uint8_t speedOfPatterns;

extern volatile uint16_t tick10ms;

void potentiometer_init(void);

void init_adc(void);
void timer_b1_10ms_init(void);





#endif /* SRC_POTENTIOMETER_H_ */
