#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RTC_VIEW_YEAR = 0,
    RTC_VIEW_MONTH,
    RTC_VIEW_DATE,
    RTC_VIEW_HOURS,
    RTC_VIEW_MINUTES,
    RTC_VIEW_SECONDS,
    RTC_VIEW_COUNT
} rtc_view_t;

typedef struct {
    uint8_t seconds_bcd;
    uint8_t minutes_bcd;
    uint8_t hours_bcd;
    uint8_t date_bcd;
    uint8_t month_bcd;
    uint8_t year_bcd;
} rtc_regs_bcd_t;

// Initialize RTC (enable battery backup, ensure oscillator running).
bool rtc_init(void);

// Refresh cached BCD registers from RTC (reads 0x00..0x06).
bool rtc_refresh_cache(void);

// Get a cached register value in BCD (byte).
uint8_t rtc_get_view_bcd(rtc_view_t view);

// Set time/date registers (expects BCD bytes, masked as needed).
bool rtc_set_regs_bcd(const rtc_regs_bcd_t *regs);

#endif // RTC_H
