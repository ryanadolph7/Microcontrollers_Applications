#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "temp_sensor.h"

#define CURRENT_WINDOW_SIZE 5
volatile unsigned int ADC_VAL = 0;
volatile float temp = 0.0f;
volatile int avg_counter = 0;
extern volatile int average_window_size = 15;
volatile float ADC_AVG[15];
volatile float moving_average = 0.0f;
volatile char AVG_TEMP[4];

// this is just a function that turns the moving average float into a char[] (to 2 decimal places)

void float_to_str_2dp(float value, char *buffer)
{
    int whole;
    int frac;
    int i = 0;
    // Handle negative numbers
    if (value < 0){
        buffer[i++] = '-';
        value = -value;
        }
    // Extract integer part
    whole = (int)value;
    // Extract fractional part (rounded to 2 decimals)
    frac = (int)((value - whole) * 100 + 0.5);
    // Convert integer part to string (manual)
    char temp[10];
    int j = 0;
    if (whole == 0){
        temp[j++] = '0';
    } else {
        while (whole > 0) {
            temp[j++] = (whole % 10) + '0';
            whole /= 10;
        }
    }
    // Reverse integer digits into buffer
    while (j > 0) {
        buffer[i++] = temp[--j];
    }
    // Decimal point
    buffer[i++] = '.';
    // Two fractional digits
    buffer[i++] = (frac / 10) + '0';
    buffer[i++] = (frac % 10) + '0';
    buffer[i] = '\0';
}

// ------------------------------------------------
// Temp sensor init
// ------------------------------------------------

/*
Temp sensor output voltage is inversaly proportional to temperature 
20 C = 68 F ~ 2365 mV
50 C = 122 F ~ 1958 mV

Calc from 20 C to 50 C for finding output voltage in mV

V - 2365 (mV) = ( (1958 - 2365 (mV)) / (50 - 20 (C)) ) *  (T - 20 (C))
V = (-13.6 (mV / C)) * T + 2637 mV

*/

void temp_sensor_init(void) {
    ADCCTL0 &= ~ADCSHT; 			// clear ADCSHT from def. of ADCSHT = 01
	ADCCTL0 |= ADCSHT_1; 			// conversion cycles = 16 (ADCSHT = 10)
    // must be at least 8 clock cycles for all resolutions 
	ADCCTL0 |= ADCON; 				// turn ADC on

	ADCCTL1 |= ADCSSEL_2; 			// ADC Clock Source = ACLK
	ADCCTL1 |= ADCSHP;				// sample signal source = sampling timer

	ADCCTL2 &= ~ADCRES;				// clear ADCRES from def. of ADCRES = 01
	ADCCTL2 |= ADCRES_2;			// resolution = 12-bit (ADCRES = 10)

	ADCMCTL0 |= ADCINCH_1;			// ADC input channel = A1 (P1.1)

	ADCIE |= ADCIE0;				// enable ADC conv complete IRQ
    return;
}

void read_temp_sensor(void) {
    ADCCTL0 |= ADCENC | ADCSC;	// enable and start conversion
    int i = 0;
    for (i; i < 100; i++) {     // wait for ADC conversion  to complete
    }
}

float square_root(float n) {
    if (n < 0) {
        return -1;
    }
    if (n == 0) {
        return 0;
    }
    float x = n;
    float y = 1.0f;
    float precision = 0.0001f;
    while(x - y > precision) {
        x = (x + y) / 2;
        y = n / x;
    }
    return x;
}

#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void) {
	ADC_VAL = ADCMEM0;			// read ADC value
    float numerator = 13.583 - (square_root((-13.582 * -13.582) + (4 * 0.00433 * (2230.8 - ADC_VAL))));
    float dividend = 2 * (-0.00433);
    temp = (numerator / dividend) + 30;
    temp = temp * (-1);
    ADC_AVG[avg_counter % average_window_size] = temp;
    avg_counter++;
    int i;
    moving_average = 0;

    for (i = 0; i < average_window_size; i++) {
        moving_average += ADC_AVG[i];
    }

    moving_average = moving_average / average_window_size;
    float_to_str_2dp(moving_average, AVG_TEMP);
} 