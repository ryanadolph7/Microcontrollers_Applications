#include <stdint.h>
#include <stdbool.h>

#include "rtc.h"
#include "i2c_controller.h"

// This driver targets the Microchip MCP7940N (common in EELE 465 projects).
// 7-bit I2C address: 0x6F
// Time/date registers start at 0x00.
// - Seconds register bit7 = ST (start oscillator)
// - Day register bit3 = VBATEN (enable battery backup)
//
// If you are using a different RTC, tell me which part and I’ll adapt the register map.

#define MCP7940N_ADDR_7   0x6Fu

#define REG_SECONDS       0x00u
#define REG_MINUTES       0x01u
#define REG_HOURS         0x02u
#define REG_DAY           0x03u
#define REG_DATE          0x04u
#define REG_MONTH         0x05u
#define REG_YEAR          0x06u

#define SECONDS_ST_BIT    0x80u
#define DAY_VBATEN_BIT    0x08u

static rtc_regs_bcd_t g_cache;

static bool rtc_read_block(uint8_t start_reg, uint8_t *buf, uint16_t len)
{
    return i2c_write_then_read(MCP7940N_ADDR_7, start_reg, buf, len);
}

static bool rtc_write_block(uint8_t start_reg, const uint8_t *buf, uint16_t len)
{
    // MCP7940N supports writing starting at a register pointer (sequential write).
    // We send [start_reg, data...]
    uint8_t tmp[1 + 8];
    uint16_t i;

    if (len > 8) return false;
    tmp[0] = start_reg;
    for (i = 0; i < len; i++) tmp[1 + i] = buf[i];

    return i2c_write(MCP7940N_ADDR_7, tmp, (uint16_t)(1u + len));
}

bool rtc_init(void)
{
    // 1) Read seconds + day
    uint8_t sec;
    uint8_t day;

    if (!rtc_read_block(REG_SECONDS, &sec, 1)) return false;
    if (!rtc_read_block(REG_DAY, &day, 1)) return false;

    // 2) Ensure oscillator is running (ST=1)
    if ((sec & SECONDS_ST_BIT) == 0) {
        uint8_t newsec = (uint8_t)(sec | SECONDS_ST_BIT);
        if (!rtc_write_block(REG_SECONDS, &newsec, 1)) return false;
    }

    // 3) Enable battery backup (VBATEN=1)
    if ((day & DAY_VBATEN_BIT) == 0) {
        uint8_t newday = (uint8_t)(day | DAY_VBATEN_BIT);
        if (!rtc_write_block(REG_DAY, &newday, 1)) return false;
    }

    // 4) Prime cache
    return rtc_refresh_cache();
}

bool rtc_refresh_cache(void)
{
    // Read seconds..year (0x00..0x06). We ignore "day" but reading it is fine.
    uint8_t buf[7];

    if (!rtc_read_block(REG_SECONDS, buf, 7)) return false;

    g_cache.seconds_bcd = (uint8_t)(buf[0] & 0x7Fu);   // clear ST bit for display
    g_cache.minutes_bcd = (uint8_t)(buf[1] & 0x7Fu);

    // Hours: keep 24h format bits as-is; mask to BCD + mode bits.
    g_cache.hours_bcd   = buf[2];

    // buf[3] = day (contains VBATEN). Not displayed per requirements list.
    g_cache.date_bcd    = (uint8_t)(buf[4] & 0x3Fu);
    g_cache.month_bcd   = (uint8_t)(buf[5] & 0x1Fu);
    g_cache.year_bcd    = buf[6];

    return true;
}

uint8_t rtc_get_view_bcd(rtc_view_t view)
{
    switch (view) {
        case RTC_VIEW_YEAR:    return g_cache.year_bcd;
        case RTC_VIEW_MONTH:   return g_cache.month_bcd;
        case RTC_VIEW_DATE:    return g_cache.date_bcd;
        case RTC_VIEW_HOURS:   return g_cache.hours_bcd;
        case RTC_VIEW_MINUTES: return g_cache.minutes_bcd;
        case RTC_VIEW_SECONDS: return g_cache.seconds_bcd;
        default:               return 0;
    }
}

bool rtc_set_regs_bcd(const rtc_regs_bcd_t *regs)
{
    // Write seconds..year in one shot.
    // Ensure ST=1 in seconds.
    uint8_t buf[7];

    buf[0] = (uint8_t)((regs->seconds_bcd & 0x7Fu) | SECONDS_ST_BIT);
    buf[1] = (uint8_t)(regs->minutes_bcd & 0x7Fu);
    buf[2] = regs->hours_bcd;              // caller provides correct format
    buf[3] = (uint8_t)(DAY_VBATEN_BIT);    // at minimum set VBATEN; day-of-week not required by rubric
    buf[4] = (uint8_t)(regs->date_bcd & 0x3Fu);
    buf[5] = (uint8_t)(regs->month_bcd & 0x1Fu);
    buf[6] = regs->year_bcd;

    if (!rtc_write_block(REG_SECONDS, buf, 7)) return false;

    // refresh cache
    return rtc_refresh_cache();
}
