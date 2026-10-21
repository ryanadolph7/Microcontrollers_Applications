#ifndef GYRO_CONTROLLER_H
#define GYRO_CONTROLLER_H
#include <stdint.h>

extern volatile int16_t gyro_x, gyro_y, gyro_z;
extern volatile int16_t accel_x, accel_y, accel_z;
extern volatile float accel_x_g, accel_y_g, accel_z_g;
extern volatile float gyro_x_dps, gyro_y_dps, gyro_z_dps;
extern int32_t average_gyro_x, average_gyro_y, average_gyro_z;

void float_to_str_1dp(float value, char *buffer);
void convert_data(void);
void read_gyro(uint8_t start_register, uint8_t *data, uint8_t length);
void spi_init(void);
uint8_t spi_write(uint8_t data);
uint8_t spi_read(uint8_t register);
void imu_init(void);
void write_gyro_data(void);

#endif