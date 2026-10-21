#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

extern volatile unsigned int ADC_VAL;
extern volatile float temp;
extern volatile int avg_counter;
extern volatile int average_window_size;
extern volatile float ADC_AVG[15];
extern volatile float moving_average;
extern volatile char AVG_TEMP[4];

void float_to_str_2dp(float value, char *buffer);
void temp_sensor_init(void);
void read_temp_sensor(void);
float square_root(float n);

#endif