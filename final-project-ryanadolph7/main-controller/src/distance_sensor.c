#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "distance_sensor.h"
#include "lcd_contrast.h"

#define PWM_INPUT        BIT0       // using P2.0 for PWM input

// only variable needed for our printing to the LCD screen 
volatile int distance = 0;
/**
 * Function to write distance data to LCD screen
 * - if distance is > 1600 the sensor cannot see it / our PWM is just at its out of range pulse 
 * - if distance is < 90 the sensor doesnt need to read / output that value anymore, just print too close
 * - write to the fourth line of the LCD screen
 */
void write_distance_data(void) {
    char distance_buff[20];
    distance_buff[0] = '\0';
    append_string(distance_buff, "Distance: ");
    if (distance > 1600 ) {
        append_string(distance_buff, "NO OBJECT");
    } else if (distance < 90) {
        append_string(distance_buff, "TOO CLOSE");
    }
    else {
        append_int(distance_buff, (unsigned int)distance);
        append_string(distance_buff, " mm     ");
    }

    write_line(LINE_4, distance_buff);
}

/**
 * config the distance sensor input pin and initial clock reading
 * - configed to read the first clock high reading to start the pulse width calculation
 * - later configed to read the low transition
 * Pins: 
 * - P2.0 - PWM input
 */
void distance_sensor_init(void) {
    P2DIR &= ~PWM_INPUT;        // setting up P2.0 as input for PWM distance sensor
    P2SEL0 |= PWM_INPUT;
    P2SEL1 &= ~PWM_INPUT;
    TB1CTL |= TBSSEL__SMCLK;
    TB1CTL |= MC__CONTINUOUS;    // use continuoues and small clock
    TB1CTL |= TBCLR;
    TB1CCTL1 |= CM_1;        // capture on rising edge
    TB1CCTL1 |= CCIS_0;      // use P2.0
    TB1CCTL1 |= SCS;
    TB1CCTL1 |= CAP;
    TB1CCTL1 |= CCIE;        // enable interrupt
    return;
}

/** 
 * Interrupt that triggers on initial high transition from distance sensor
 * - configed to triggerr on high-to-low transition after rise has been read  
 * - distance conversion is 
 *  - width = rise - fall time
 *  - distance = (width - 1000) * 2 --> documentation uses this but for some reason my reading is off by 3.9 inches so might need to subtract more or adjust the clock? 
 */
#pragma vector = TIMER1_B1_VECTOR
__interrupt void TIMER1_B1_ISR(void) {      // off by about 3.9 inches, need to fix time some math
    static int rising = 0;
    static int falling = 0;
    static int waiting = 0;
    uint16_t time_captured = 0;

    switch(TB1IV) {
        case TB1IV_TBCCR1:
            time_captured = TB1CCR1;
            if(waiting == 0) {      // we have captured the rising edge, now we are waiting for the falling edge
                rising = time_captured;
                waiting = 1;
                TB1CCTL1 &= ~CM_3;          // switch to capturinig on the falling edge
                TB1CCTL1 |= CM_2;
            } else {
                falling = time_captured;
                int width = falling - rising;           // adjust these values to get correct reading
                distance = (width - 1000) * 2;
                waiting = 0;
                TB1CCTL1 &= ~CM_3;          // switch back to capture on the rising edge
                TB1CCTL1 |= CM_1;       
            }

            break;
        default:
            break;
    }


}