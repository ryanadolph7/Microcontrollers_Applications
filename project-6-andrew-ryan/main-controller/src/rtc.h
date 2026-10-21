#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <stdbool.h>

extern volatile uint8_t rtc_month;
extern volatile uint8_t rtc_day;
extern volatile uint16_t rtc_year;
extern volatile uint8_t rtc_hour;
extern volatile uint8_t rtc_minute;
extern volatile uint8_t rtc_second;
extern volatile uint8_t rtc_ok;

void rtc_init(void);
bool rtc_update_time(void);
bool rtc_set_time_simple(uint16_t year,
                         uint8_t month,
                         uint8_t day,
                         uint8_t hour,
                         uint8_t minute,
                         uint8_t second,
                         uint8_t wday);

#endif
