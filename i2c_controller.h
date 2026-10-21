#ifndef I2C_CONTROLLER_H
#define I2C_CONTROLLER_H

#include <msp430fr2153.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void i2c_controller_init(const uint16_t usci_bx_base_addr, const uint16_t port_sel0_addr, const uint8_t sda_pin, const uint8_t scl_pin);

bool i2c_controller_is_idle();

void i2c_controller_transmit(const uint8_t address, const uint8_t * user_tx_buffer, const uint16_t n_bytes);

void i2c_controller_receive(const uint8_t address, uint8_t * const user_rx_buffer, const uint16_t n_bytes);

#endif // I2C_CONTROLLER_H
