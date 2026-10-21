#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

extern volatile int distance;

void write_distance_data(void);
void distance_sensor_init(void);


#endif