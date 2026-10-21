#ifndef I2C_CONTROLLER_H
#define I2C_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

void i2c_init(uint32_t smclk_hz, uint32_t i2c_hz);

// Write N bytes to a slave (no register pointer convenience).
bool i2c_write(uint8_t addr7, const uint8_t *data, uint16_t len);

// Write register pointer, then read N bytes (common I2C register read pattern).
bool i2c_write_then_read(uint8_t addr7, uint8_t reg, uint8_t *rx, uint16_t rx_len);

#endif // I2C_CONTROLLER_H
