#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#define CURRENT_WINDOW_SIZE 5
unsigned int ADC_VAL;
float temp;
int avg_counter = 0;
int average_window_size = 5;
float ADC_AVG[16];
float moving_average = 0.0;
volatile uint8_t rx_idx = 0;
volatile uint8_t g_window_size = 5;
static volatile char rx_line[32];
static volatile bool rx_line_ready = false;

int data_cnt = 0;
char packet[] = {0x03, 0x01, 0x02, 0x03};
//char packet[] = {0x03, 0x00, 0x00, 0x00, 0x09, 0x02, 0x01, 0x03, 0x1A};
            // {start addr, sec, min, hour, day, weekday, month, 2-dig year}

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

	ADCMCTL0 |= ADCINCH_5;			// ADC input channel = A5 (P1.5)

	ADCIE |= ADCIE0;				// enable ADC conv complete IRQ
    return;
}

void uart_init(void)
{
    // Put eUSCI_A0 in reset
    UCA0CTLW0 |= UCSWRST;

    // Select SMCLK
    UCA0CTLW0 |= UCSSEL__SMCLK;

    // 1 MHz / 9600 = 104.1667
    UCA0BRW = 104;

    // Modulation settings for 9600 baud @ 1MHz
    UCA0MCTLW = UCOS16 | UCBRF_2 | 0xD600;

    // Release from reset
    UCA0CTLW0 &= ~UCSWRST;
}

void i2c_init(void) {
    UCB0CTLW0 |= UCSWRST;       // UCSWRST = 1 for eUSCI_B0 in SW reset
    UCB0CTLW0 |= UCSSEL__SMCLK;     // chose BRCLK = SMCLK = 1MHz
    UCB0BRW = 10;                   // divide BRCLK by 10 for SCL = 100kHz
    UCB0CTLW0 |= UCMODE_3;          // put into i2c mode
    UCB0CTLW0 |= UCMST;         // put into master mode
    UCB0CTLW0 |= UCTR;          // put into tx mode
    UCB0I2CSA |= 0x0068;        // test slave address
    UCB0CTLW1 |= UCASTP_2;      // auto stop when ucb0tbctn reached
    UCB0TBCNT = sizeof(packet);
    P1SEL1 &= ~BIT3;        // P1.3 = SCL;
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;        // P1.2 = SDA
    P1SEL0 |= BIT2;


}

// -----------------------------------------------------
// got this from chat GPT / our last project 
void uart_send_char(char c)
{
    while (!(UCA0IFG & UCTXIFG));  // Wait for TX buffer ready
    UCA0TXBUF = c;
}
void uart_send_string(const char *str)
{
    while (*str)
    {
        uart_send_char(*str++);
    }
}

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

static uint8_t parse_window_size_line(const char *s)
{
    // Expect "WIN:<n>"
    if (!(s[0]=='W' && s[1]=='I' && s[2]=='N' && s[3]==':'))
        return 0; // 0 means "not a window size command"

    uint16_t n = 0;
    const char *p = &s[4];

    // parse decimal
    while (*p >= '0' && *p <= '9')
    {
        n = (uint16_t)(n * 10u + (uint16_t)(*p - '0'));
        p++;
    }

    // enforce bounds (your GUI uses 1..16)
    if (n < 1) n = 1;
    if (n > 16) n = 16;

    return (uint8_t)n;
}

// ------------------------------------------------

int main(void)
{
    // Stop the watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // uart init 
    
    UCA0CTLW0 |= UCSWRST;              // hold in reset
    UCA0CTLW0 = UCSWRST | UCSSEL__SMCLK;
    UCA0BRW = 8;            // 115200 baud rate
    UCA0MCTLW |= 0xD600;

    P1SEL1 &= ~(BIT6 | BIT7);       // uart pins
    P1SEL0 |= (BIT6 | BIT7);

    //i2c_init();
    // Disable low-power mode
    PM5CTL0 &= ~LOCKLPM5;
    // ^^^^^^^^^^^^^^^^
    // turn on / config ADC after turning on GPIO 
    UCA0CTLW0 &= ~UCSWRST;
    UCA0IE |= UCRXIE;       // enable rx interrupt
    //UCB0CTLW0 &= ~UCSWRST;      // ucswrst = 1 for eusci_b0 in sw reset
    //UCB0IE |= UCTXIE0;      // enable i2c TX interrupt

    temp_sensor_init();
	__enable_interrupt();			// enable maskable interrupts
    
    while(true)
    {
        int i;
        ADCCTL0 |= ADCENC | ADCSC;	// enable and start conversion
        char average = (char)moving_average;
        char buffer[32];
        float_to_str_2dp(moving_average, buffer);
        uart_send_string(buffer);
        uart_send_string("\r\n");
        __delay_cycles(10000);

        if (rx_line_ready)
        {
            // Copy the volatile buffer safely
            char local[32];
            __disable_interrupt();
            uint8_t i;
            for (i = 0; i < sizeof(local); i++) {
                local[i] = (char)rx_line[i];
            }
            rx_line_ready = false;
            __enable_interrupt();
            uint8_t ws = parse_window_size_line(local);
            if (ws != 0)
            {
                average_window_size = ws;
            }
        }
        //UCB0CTLW0 |= UCTXSTT;       // generate I2C start
        //int j; 
        //for (j = 0; j < 10000; j++) {
        //}
        
        
    }
    return 0;
}

// ------------------------------------------------------------------------------
// attempt at I2C transmission
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_TX(void) {
    if(data_cnt == (sizeof(packet) -1)) {
        //UCB0TXBUF = packet[data_cnt];
        data_cnt = 0;
    } else {
        UCB0TXBUF = data_cnt;
        //UCB0TXBUF = packet[data_cnt];
        data_cnt++;
    }
    UCB0IFG &= ~UCTXIFG0;
}

// ----------------------------------------------------------------------------------------
// Temp sensor interrupt that calculates temperature reading from analog temp sensor
#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void) {
    //if 
	ADC_VAL = ADCMEM0;			// read ADC value
    float numerator = 13.583 - (sqrt(pow(-13.582, 2) + (4 * 0.00433 * (2230.8 - ADC_VAL))));
    float dividend = 2 * (-0.00433);
    temp = (numerator / dividend) + 30;
    temp = temp * (-1);
    ADC_AVG[avg_counter % average_window_size] = temp;
    avg_counter++;
    // some moving average mod stuff
    int i;
    moving_average = 0;

    for (i = 0; i < average_window_size; i++) {
        moving_average += ADC_AVG[i];
    }

    moving_average = moving_average / average_window_size;
} 


// ---------------------------------------------------------------------
// reading in UART data with simple parsing - got from GPT / our last project
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch (__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;

        case USCI_UART_UCRXIFG:
        {
            char c = (char)UCA0RXBUF;

            // optional: ignore CR
            if (c == '\r') break;

            if (!rx_line_ready)
            {
                if (c == '\n')
                {
                    // terminate line
                    if (rx_idx < sizeof(rx_line))
                        rx_line[rx_idx] = '\0';
                    else
                        rx_line[sizeof(rx_line) - 1] = '\0';

                    rx_line_ready = true;
                    rx_idx = 0;
                }
                else
                {
                    // store char if room, else reset (avoid overflow)
                    if (rx_idx < (sizeof(rx_line) - 1))
                        rx_line[rx_idx++] = c;
                    else
                        rx_idx = 0; // simple overflow handling
                }
            }
            break;
        }

        default: break;
    }
}