#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "i2c_controller.h"

static void i2c_pins_to_i2c_mode(void)
{
    // Set pins to I2C function (SEL1=1, SEL0=0 on many FR2xx parts).
    // If your part uses a different mapping, adjust here.
    I2C_SDA_PORT_SEL0 &= (uint8_t)~I2C_SDA_BIT;
    I2C_SDA_PORT_SEL1 |= I2C_SDA_BIT;

    I2C_SCL_PORT_SEL0 &= (uint8_t)~I2C_SCL_BIT;
    I2C_SCL_PORT_SEL1 |= I2C_SCL_BIT;
}

void i2c_init(uint32_t smclk_hz, uint32_t i2c_hz)
{
    uint16_t br;

    i2c_pins_to_i2c_mode();

    // Hold eUSCI in reset while configuring
    I2C_UCBx_CTLW0 = UCSWRST;

    // I2C master, synchronous, SMCLK
    I2C_UCBx_CTLW0 |= UCMST | UCMODE_3 | UCSYNC | UCSSEL__SMCLK;

    // Clock divider
    if (i2c_hz == 0) i2c_hz = 100000UL;
    br = (uint16_t)(smclk_hz / i2c_hz);
    if (br < 2) br = 2;
    I2C_UCBx_BRW = br;

    // Auto STOP after TBCNT bytes
    I2C_UCBx_CTLW1 = UCASTP_2;

    // Clear flags
    I2C_UCBx_IFG = 0;

    // Release reset
    I2C_UCBx_CTLW0 &= (uint16_t)~UCSWRST;
}

static bool i2c_wait_flag(uint16_t mask, uint32_t timeout)
{
    while (((I2C_UCBx_IFG & mask) == 0) && timeout) {
        timeout--;
    }
    return (I2C_UCBx_IFG & mask) != 0;
}

bool i2c_write(uint8_t addr7, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (len == 0) return true;

    while (I2C_UCBx_STATW & UCBBUSY) { /* wait */ }

    I2C_UCBx_I2CSA = addr7;
    I2C_UCBx_TBCNT = len;

    // TX mode + START
    I2C_UCBx_CTLW0 |= UCTR | UCTXSTT;

    for (i = 0; i < len; i++) {
        if (!i2c_wait_flag(UCTXIFG0, 200000UL)) return false;

        if (I2C_UCBx_IFG & UCNACKIFG) {
            I2C_UCBx_IFG &= (uint16_t)~UCNACKIFG;
            I2C_UCBx_CTLW0 |= UCTXSTP;
            return false;
        }

        I2C_UCBx_TXBUF = data[i];
    }

    if (!i2c_wait_flag(UCSTPIFG, 200000UL)) return false;
    I2C_UCBx_IFG &= (uint16_t)~UCSTPIFG;

    return true;
}

bool i2c_write_then_read(uint8_t addr7, uint8_t reg, uint8_t *rx, uint16_t rx_len)
{
    uint8_t regbuf[1];
    uint16_t i;

    regbuf[0] = reg;

    if (!i2c_write(addr7, regbuf, 1)) return false;
    if (rx_len == 0) return true;

    while (I2C_UCBx_STATW & UCBBUSY) { /* wait */ }

    I2C_UCBx_I2CSA = addr7;
    I2C_UCBx_TBCNT = rx_len;

    // RX mode + START
    I2C_UCBx_CTLW0 &= (uint16_t)~UCTR;
    I2C_UCBx_CTLW0 |= UCTXSTT;

    for (i = 0; i < rx_len; i++) {
        if (!i2c_wait_flag(UCRXIFG0, 200000UL)) return false;

        if (I2C_UCBx_IFG & UCNACKIFG) {
            I2C_UCBx_IFG &= (uint16_t)~UCNACKIFG;
            I2C_UCBx_CTLW0 |= UCTXSTP;
            return false;
        }

        rx[i] = I2C_UCBx_RXBUF;
    }

    if (!i2c_wait_flag(UCSTPIFG, 200000UL)) return false;
    I2C_UCBx_IFG &= (uint16_t)~UCSTPIFG;

    return true;
}
