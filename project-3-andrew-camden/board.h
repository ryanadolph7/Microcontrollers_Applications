#ifndef BOARD_H
#define BOARD_H

/*
 * Centralized pin assignments.
 *
 * Adjust these to match your wiring.
 *
 * Notes:
 * - This code assumes buttons are wired to GND and use internal pull-ups.
 * - Enable pins and indicator LEDs are assumed ACTIVE HIGH.
 * - LED bar mux/color port uses P4.0-7 by default (lower nibble = RED select, upper nibble = GREEN select).
 */

#if defined(__MSP430FR2355__)
  #include <msp430fr2355.h>
#elif defined(__MSP430FR2153__)
  #include <msp430fr2153.h>
#else
  #include <msp430.h>
#endif

// -------- LED bar (multiplexed bi-color, 12 segments grouped 4+4+4) --------
#define LEDMUX_PORT_DIR   P4DIR
#define LEDMUX_PORT_OUT   P4OUT
#define LEDMUX_RED_MASK   (BIT3|BIT2|BIT1|BIT0)   // P4.0-3
#define LEDMUX_GRN_MASK   (BIT7|BIT6|BIT5|BIT4)   // P4.4-7
#define LEDMUX_ALL_MASK   (LEDMUX_RED_MASK|LEDMUX_GRN_MASK)

#define LEDEN_PORT_DIR    P3DIR
#define LEDEN_PORT_OUT    P3OUT
#define LEDEN_LO_BIT      BIT7   // P3.7 (lower 4)
#define LEDEN_MID_BIT     BIT6   // P3.6 (middle 4)
#define LEDEN_HI_BIT      BIT5   // P3.5 (upper 4)

// Red select nibble wiring is reversed relative to green (your fix #3)
#define RED_NIBBLE_REVERSED 1

// -------- Buttons (Port 3) --------
#define BTN_PORT_DIR      P3DIR
#define BTN_PORT_REN      P3REN
#define BTN_PORT_OUT      P3OUT
#define BTN_PORT_IN       P3IN
#define BTN_PORT_IE       P3IE
#define BTN_PORT_IES      P3IES
#define BTN_PORT_IFG      P3IFG

#define BTN_MODE_BIT      BIT2   // P3.2 : toggles Patterns <-> RTC mode
#define BTN_NEXT_BIT      BIT4   // P3.4 : next pattern / next RTC register
#define BTN_COLOR_BIT     BIT3   // P3.3 : cycle color (extra credit / convenience)

// -------- Indicator LEDs --------
// Heartbeat LED (Requirement 1)
#define HB_PORT_DIR       P1DIR
#define HB_PORT_OUT       P1OUT
#define HB_BIT            BIT0   // P1.0

// Mode indicator LED (Requirement 2): OFF=patterns, ON=RTC
#define MODELED_PORT_DIR  P1DIR
#define MODELED_PORT_OUT  P1OUT
#define MODELED_BIT       BIT1   // P1.1

// -------- I2C (eUSCI_B0) --------
// Default pins (common on FR2xx/FR4xx): P1.2 = SDA, P1.3 = SCL
#define I2C_SDA_PORT_SEL0 P1SEL0
#define I2C_SDA_PORT_SEL1 P1SEL1
#define I2C_SCL_PORT_SEL0 P1SEL0
#define I2C_SCL_PORT_SEL1 P1SEL1
#define I2C_SDA_BIT       BIT2
#define I2C_SCL_BIT       BIT3

// -------- UART (eUSCI_A0) --------
#define UART_PORT_SEL0    P1SEL0
#define UART_PORT_SEL1    P1SEL1
#define UART_TX_BIT       BIT4    // P1.4 = UCA0TXD
#define UART_RX_BIT       BIT5    // P1.5 = UCA0RXD

// Use eUSCI_B0
#define I2C_UCBx_CTLW0    UCB0CTLW0
#define I2C_UCBx_CTLW1    UCB0CTLW1
#define I2C_UCBx_BRW      UCB0BRW
#define I2C_UCBx_I2CSA    UCB0I2CSA
#define I2C_UCBx_TBCNT    UCB0TBCNT
#define I2C_UCBx_TXBUF    UCB0TXBUF
#define I2C_UCBx_RXBUF    UCB0RXBUF
#define I2C_UCBx_IFG      UCB0IFG
#define I2C_UCBx_STATW    UCB0STATW

#endif // BOARD_H
