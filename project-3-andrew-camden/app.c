#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "app.h"
#include "led_bar.h"
#include "led_patterns.h"
#include "i2c_controller.h"
#include "rtc.h"
#include "uart.h"

#define RTC_PRESENT 0   // set to 1 when RTC hardware is connected

// ---- UART time helpers (prototypes required for C89) ----
static uint8_t dec_to_bcd(uint8_t v);
static uint8_t bcd_to_dec(uint8_t b);
static void    uart_put_u2(uint8_t v);
static void    uart_print_datetime_bcd(void);

// App modes
typedef enum {
    MODE_PATTERNS = 0,
    MODE_RTC
} app_mode_t;

static volatile uint32_t g_ms = 0;

// debouncing
#define DEBOUNCE_MS 30u

static volatile uint32_t g_last_mode_press = 0;
static volatile uint32_t g_last_next_press = 0;
static volatile uint32_t g_last_color_press = 0;

// color state (applies to both modes for display)
static volatile led_color_t g_color = LED_COLOR_GREEN;

// mode + rtc view selection
static volatile app_mode_t g_mode = MODE_PATTERNS;
static volatile rtc_view_t g_rtc_view = RTC_VIEW_YEAR;

// flags to run work in main context
static volatile bool g_flag_update_display = true;
static volatile bool g_flag_rtc_refresh = false;

// heartbeat timing
static volatile uint32_t g_last_hb_toggle = 0;

// rtc refresh timing
#define RTC_REFRESH_MS 500u

static void set_mode_indicator(app_mode_t mode)
{
    if (mode == MODE_RTC) MODELED_PORT_OUT |= MODELED_BIT;
    else                 MODELED_PORT_OUT &= (uint8_t)~MODELED_BIT;
}

static void update_display_now(void)
{
    if (g_mode == MODE_PATTERNS) {
        led_bar_set_mask10(led_patterns_get_mask10(), g_color);
    } else {
        uint8_t bcd;
        uint16_t mask10;

        bcd = rtc_get_view_bcd(g_rtc_view);
        mask10 = (uint16_t)(bcd & 0xFFu);   // bits0..7
        led_bar_set_mask10(mask10, g_color);
    }
}

void app_init(void)
{
    // Indicator LEDs
    HB_PORT_DIR |= HB_BIT;
    HB_PORT_OUT &= (uint8_t)~HB_BIT;

    MODELED_PORT_DIR |= MODELED_BIT;
    MODELED_PORT_OUT &= (uint8_t)~MODELED_BIT;

    set_mode_indicator(MODE_PATTERNS);

    // Buttons (pull-ups, interrupt on falling edge)
    BTN_PORT_DIR &= (uint8_t)~(BTN_MODE_BIT | BTN_NEXT_BIT | BTN_COLOR_BIT);
    BTN_PORT_REN |= (BTN_MODE_BIT | BTN_NEXT_BIT | BTN_COLOR_BIT);
    BTN_PORT_OUT |= (BTN_MODE_BIT | BTN_NEXT_BIT | BTN_COLOR_BIT);

    BTN_PORT_IES |= (BTN_MODE_BIT | BTN_NEXT_BIT | BTN_COLOR_BIT);
    BTN_PORT_IFG &= (uint8_t)~(BTN_MODE_BIT | BTN_NEXT_BIT | BTN_COLOR_BIT);
    BTN_PORT_IE  |= (BTN_MODE_BIT | BTN_NEXT_BIT | BTN_COLOR_BIT);

    // LED bar + patterns
    led_bar_init();
    led_patterns_init();

    #if RTC_PRESENT
        i2c_init(1000000UL, 100000UL);
        rtc_init();
        rtc_refresh_cache();
    #else
        uart_puts("RTC not present: skipping I2C/RTC init.\r\n");
    #endif


    // UART INIT
    uart_init(1000000UL, 9600UL);
    uart_puts("\r\nProject 3 UART ready.\r\n");
    uart_puts("Commands:\r\n");
    uart_puts("  GET\r\n");
    uart_puts("  SET yy mm dd hh mm ss\r\n");
    uart_puts("\r\n");

    g_flag_update_display = true;
}

static bool debounce_ok(volatile uint32_t *last_ms)
{
    uint32_t now;
    now = g_ms;
    if ((now - *last_ms) < DEBOUNCE_MS) return false;
    *last_ms = now;
    return true;
}

bool app_isr_tick_1ms(void)
{
    bool wake;
    wake = false;

    g_ms++;

    // keep scanning the LED bar
    led_bar_isr_scan_1ms();

    // heartbeat @ 1Hz (toggle every 500ms)
    if ((g_ms - g_last_hb_toggle) >= 500u) {
        HB_PORT_OUT ^= HB_BIT;
        g_last_hb_toggle = g_ms;
    }

    // pattern stepping
    if (g_mode == MODE_PATTERNS) {
        if (led_patterns_on_tick_1ms()) {
            g_flag_update_display = true;
        }
    }

    // schedule RTC refresh (avoid modulo in ISR for CGT restrictions / power warnings)
    //static uint16_t rtc_div = 0;
    //rtc_div++;
    //if (rtc_div >= (uint16_t)RTC_REFRESH_MS) {
    //    rtc_div = 0;
    //    g_flag_rtc_refresh = true;
    //    wake = true; // need main to do I2C
    //}

    if (g_flag_update_display) {
        wake = true;
    }

    return wake;
}

bool app_isr_port3(void)
{
    bool wake;
    uint8_t ifg;

    wake = false;
    ifg = BTN_PORT_IFG;

    // MODE button
    if (ifg & BTN_MODE_BIT) {
        BTN_PORT_IFG &= (uint8_t)~BTN_MODE_BIT;

        if (debounce_ok(&g_last_mode_press)) {
            g_mode = (g_mode == MODE_PATTERNS) ? MODE_RTC : MODE_PATTERNS;
            set_mode_indicator(g_mode);
            g_flag_update_display = true;
            wake = true;
        }
    }

    // NEXT button
    if (ifg & BTN_NEXT_BIT) {
        BTN_PORT_IFG &= (uint8_t)~BTN_NEXT_BIT;

        if (debounce_ok(&g_last_next_press)) {
            if (g_mode == MODE_PATTERNS) {
                led_patterns_next();
            } else {
                g_rtc_view = (rtc_view_t)((uint8_t)g_rtc_view + 1u);
                if (g_rtc_view >= RTC_VIEW_COUNT) g_rtc_view = RTC_VIEW_YEAR;
            }
            g_flag_update_display = true;
            wake = true;
        }
    }

    // COLOR button
    if (ifg & BTN_COLOR_BIT) {
        BTN_PORT_IFG &= (uint8_t)~BTN_COLOR_BIT;

        if (debounce_ok(&g_last_color_press)) {
            if (g_color == LED_COLOR_RED) g_color = LED_COLOR_GREEN;
            else if (g_color == LED_COLOR_GREEN) g_color = LED_COLOR_YELLOW;
            else g_color = LED_COLOR_RED;

            g_flag_update_display = true;
            wake = true;
        }
    }

    return wake;
}

static uint32_t last_uart_ms = 0;

void app_task(void)
{
    uint32_t now = g_ms;

    #if RTC_PRESENT
        if (g_flag_rtc_refresh) {
            g_flag_rtc_refresh = false;
            rtc_refresh_cache();
            if (g_mode == MODE_RTC) g_flag_update_display = true;
        }
    #endif

    // Req 12: UART update at least every 5s
    if ((now - last_uart_ms) >= 5000u) {
        last_uart_ms = now;
#if RTC_PRESENT
        rtc_refresh_cache();
#endif
        uart_puts("TIME: ");
        uart_print_datetime_bcd();
    }

    // Handle incoming UART commands (Req 11)
    if (uart_line_available()) {
        char line[64];
        uint16_t n = uart_readline(line, sizeof(line));
        if (n > 0) {
            // simple parsing: GET or SET yy mm dd hh mm ss
            if (line[0] == 'G' && line[1] == 'E' && line[2] == 'T') {
                uart_puts("TIME: ");
                uart_print_datetime_bcd();
            } else if (line[0] == 'S' && line[1] == 'E' && line[2] == 'T') {
#if RTC_PRESENT
                unsigned int yy, mo, dd, hh, mi, ss;
                if (sscanf(line, "SET %u %u %u %u %u %u", &yy, &mo, &dd, &hh, &mi, &ss) == 6) {
                    rtc_regs_bcd_t r;
                    r.year_bcd    = dec_to_bcd((uint8_t)(yy % 100u));
                    r.month_bcd   = dec_to_bcd((uint8_t)mo);
                    r.date_bcd    = dec_to_bcd((uint8_t)dd);
                    r.hours_bcd   = dec_to_bcd((uint8_t)hh);
                    r.minutes_bcd = dec_to_bcd((uint8_t)mi);
                    r.seconds_bcd = dec_to_bcd((uint8_t)ss);

                    if (rtc_set_regs_bcd(&r)) {
                        uart_puts("OK\r\n");
                        uart_puts("TIME: ");
                        uart_print_datetime_bcd();
                    } else {
                        uart_puts("ERR: rtc_set_regs_bcd failed\r\n");
                    }
                } else {
                    uart_puts("ERR: usage: SET yy mm dd hh mm ss\r\n");
                }
#else
                uart_puts("ERR: RTC not present\r\n");
#endif
            } else {
                uart_puts("ERR: unknown command\r\n");
            }
        }
    }

    if (g_flag_update_display) {
        g_flag_update_display = false;
        update_display_now();
    }
}

static uint8_t dec_to_bcd(uint8_t v)
{
    return (uint8_t)(((v / 10u) << 4) | (v % 10u));
}

static uint8_t bcd_to_dec(uint8_t b)
{
    return (uint8_t)(((b >> 4) * 10u) + (b & 0x0Fu));
}

static void uart_put_u2(uint8_t v)
{
    uart_putc((char)('0' + (v / 10u)));
    uart_putc((char)('0' + (v % 10u)));
}

static void uart_print_datetime_bcd(void)
{
#if RTC_PRESENT
    uint8_t yy = bcd_to_dec(rtc_get_view_bcd(RTC_VIEW_YEAR));
    uint8_t mo = bcd_to_dec(rtc_get_view_bcd(RTC_VIEW_MONTH));
    uint8_t dd = bcd_to_dec(rtc_get_view_bcd(RTC_VIEW_DATE));
    uint8_t hh = bcd_to_dec((uint8_t)(rtc_get_view_bcd(RTC_VIEW_HOURS) & 0x3Fu));
    uint8_t mi = bcd_to_dec(rtc_get_view_bcd(RTC_VIEW_MINUTES));
    uint8_t ss = bcd_to_dec(rtc_get_view_bcd(RTC_VIEW_SECONDS));

    uart_puts("20"); uart_put_u2(yy); uart_putc('-'); uart_put_u2(mo); uart_putc('-'); uart_put_u2(dd);
    uart_putc(' ');
    uart_put_u2(hh); uart_putc(':'); uart_put_u2(mi); uart_putc(':'); uart_put_u2(ss);
    uart_puts("\r\n");
#else
    uart_puts("(RTC not present)\r\n");
#endif
}


