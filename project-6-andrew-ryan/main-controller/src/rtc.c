#include <msp430fr2153.h>
#include <stdint.h>
#include <stdbool.h>
#include "rtc.h"

#define MCP7940_ADDR          0x6F

#define MCP7940_REG_RTCSEC    0x00
#define MCP7940_REG_RTCMIN    0x01
#define MCP7940_REG_RTCHOUR   0x02
#define MCP7940_REG_RTCWKDAY  0x03
#define MCP7940_REG_RTCDATE   0x04
#define MCP7940_REG_RTCMTH    0x05
#define MCP7940_REG_RTCYEAR   0x06

#define MCP7940_ST_BIT        0x80
#define MCP7940_VBATEN_BIT    0x08

#define I2C_TIMEOUT           40000UL

volatile uint8_t rtc_month = 4;
volatile uint8_t rtc_day = 11;
volatile uint16_t rtc_year = 2026;
volatile uint8_t rtc_hour = 10;
volatile uint8_t rtc_minute = 38;
volatile uint8_t rtc_second = 0;
volatile uint8_t rtc_ok = 0;

static uint8_t bin_to_bcd(uint8_t v)
{
    return ((v / 10) << 4) | (v % 10);
}

static uint8_t bcd_to_bin(uint8_t v)
{
    return ((v >> 4) * 10) + (v & 0x0F);
}

static void rtc_i2c_abort(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    UCB0CTLW0 |= UCTXSTP;

    while ((UCB0CTLW0 & UCTXSTP) && timeout--)
    {
    }

    UCB0IFG &= ~UCNACKIFG;
}

static bool wait_for_txifg(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (!(UCB0IFG & UCTXIFG0))
    {
        if (UCB0IFG & UCNACKIFG)
        {
            rtc_i2c_abort();
            return false;
        }

        if (--timeout == 0)
        {
            rtc_i2c_abort();
            return false;
        }
    }

    return true;
}

static bool wait_for_rxifg(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (!(UCB0IFG & UCRXIFG0))
    {
        if (UCB0IFG & UCNACKIFG)
        {
            rtc_i2c_abort();
            return false;
        }

        if (--timeout == 0)
        {
            rtc_i2c_abort();
            return false;
        }
    }

    return true;
}

static bool wait_for_start_clear(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (UCB0CTLW0 & UCTXSTT)
    {
        if (UCB0IFG & UCNACKIFG)
        {
            rtc_i2c_abort();
            return false;
        }

        if (--timeout == 0)
        {
            rtc_i2c_abort();
            return false;
        }
    }

    return true;
}

static bool wait_for_stop_clear(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((UCB0CTLW0 & UCTXSTP) || (UCB0STATW & UCBBUSY))
    {
        if (--timeout == 0)
        {
            return false;
        }
    }

    return true;
}

static void rtc_i2c_init(void)
{
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    UCB0CTLW0 = UCSWRST;
    UCB0CTLW0 |= UCSSEL__SMCLK | UCMODE_3 | UCMST | UCSYNC;
    UCB0BRW = 10;
    UCB0I2CSA = MCP7940_ADDR;
    UCB0CTLW0 &= ~UCSWRST;
}

static bool i2c_write_bytes(uint8_t slave_addr, const uint8_t *data, uint8_t len)
{
    if (len == 0)
    {
        return false;
    }

    while (UCB0CTLW0 & UCTXSTP)
    {
    }

    UCB0I2CSA = slave_addr;
    UCB0IFG &= ~(UCTXIFG0 | UCRXIFG0 | UCNACKIFG);

    UCB0CTLW0 |= UCTR | UCTXSTT;

    while (len--)
    {
        if (!wait_for_txifg())
        {
            return false;
        }

        UCB0TXBUF = *data++;
    }

    if (!wait_for_txifg())
    {
        return false;
    }

    UCB0CTLW0 |= UCTXSTP;

    return wait_for_stop_clear();
}

static bool i2c_read_bytes(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    uint8_t i;

    if (len == 0)
    {
        return false;
    }

    while (UCB0CTLW0 & UCTXSTP)
    {
    }

    UCB0I2CSA = slave_addr;
    UCB0IFG &= ~(UCTXIFG0 | UCRXIFG0 | UCNACKIFG);

    UCB0CTLW0 |= UCTR | UCTXSTT;

    if (!wait_for_txifg())
    {
        return false;
    }

    UCB0TXBUF = reg_addr;

    if (!wait_for_txifg())
    {
        return false;
    }

    UCB0CTLW0 &= ~UCTR;
    UCB0CTLW0 |= UCTXSTT;

    if (len == 1)
    {
        if (!wait_for_start_clear())
        {
            return false;
        }

        UCB0CTLW0 |= UCTXSTP;

        if (!wait_for_rxifg())
        {
            return false;
        }

        data[0] = UCB0RXBUF;

        return wait_for_stop_clear();
    }

    for (i = 0; i < len; i++)
    {
        if (!wait_for_rxifg())
        {
            return false;
        }

        data[i] = UCB0RXBUF;

        if (i == (len - 2))
        {
            UCB0CTLW0 |= UCTXSTP;
        }
    }

    return wait_for_stop_clear();
}

static bool rtc_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    return i2c_write_bytes(MCP7940_ADDR, buf, 2);
}

static bool rtc_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_read_bytes(MCP7940_ADDR, reg, value, 1);
}

void rtc_init(void)
{
    uint8_t sec;
    uint8_t wday;

    rtc_i2c_init();
    rtc_ok = 0;

    if (!rtc_read_reg(MCP7940_REG_RTCSEC, &sec))
    {
        return;
    }

    if (!(sec & MCP7940_ST_BIT))
    {
        sec |= MCP7940_ST_BIT;

        if (!rtc_write_reg(MCP7940_REG_RTCSEC, sec))
        {
            return;
        }
    }

    if (!rtc_read_reg(MCP7940_REG_RTCWKDAY, &wday))
    {
        return;
    }

    if (!(wday & MCP7940_VBATEN_BIT))
    {
        wday |= MCP7940_VBATEN_BIT;

        if (!rtc_write_reg(MCP7940_REG_RTCWKDAY, wday))
        {
            return;
        }
    }

    rtc_update_time();
}

bool rtc_set_time_simple(uint16_t year, uint8_t month, uint8_t day,
                         uint8_t hour, uint8_t minute, uint8_t second,
                         uint8_t wday)
{
    uint8_t buf[8];

    if (wday < 1 || wday > 7)
    {
        wday = 1;
    }

    buf[0] = MCP7940_REG_RTCSEC;
    buf[1] = bin_to_bcd(second) & 0x7F;
    buf[2] = bin_to_bcd(minute) & 0x7F;
    buf[3] = bin_to_bcd(hour) & 0x3F;
    buf[4] = MCP7940_VBATEN_BIT | (wday & 0x07);
    buf[5] = bin_to_bcd(day) & 0x3F;
    buf[6] = bin_to_bcd(month) & 0x1F;
    buf[7] = bin_to_bcd((uint8_t)(year % 100));

    if (!i2c_write_bytes(MCP7940_ADDR, buf, 8))
    {
        rtc_ok = 0;
        return false;
    }

    buf[1] |= MCP7940_ST_BIT;

    if (!rtc_write_reg(MCP7940_REG_RTCSEC, buf[1]))
    {
        rtc_ok = 0;
        return false;
    }

    rtc_year = year;
    rtc_month = month;
    rtc_day = day;
    rtc_hour = hour;
    rtc_minute = minute;
    rtc_second = second;
    rtc_ok = 1;

    return true;
}

bool rtc_update_time(void)
{
    uint8_t buf[7];

    if (!i2c_read_bytes(MCP7940_ADDR, MCP7940_REG_RTCSEC, buf, 7))
    {
        rtc_ok = 0;
        return false;
    }

    rtc_second = bcd_to_bin(buf[0] & 0x7F);
    rtc_minute = bcd_to_bin(buf[1] & 0x7F);
    rtc_hour   = bcd_to_bin(buf[2] & 0x3F);
    rtc_day    = bcd_to_bin(buf[4] & 0x3F);
    rtc_month  = bcd_to_bin(buf[5] & 0x1F);
    rtc_year   = 2000 + bcd_to_bin(buf[6]);
    rtc_ok = 1;

    return true;
}
