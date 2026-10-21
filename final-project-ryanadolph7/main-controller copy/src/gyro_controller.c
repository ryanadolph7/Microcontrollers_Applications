#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include "gyro_controller.h"
#include "lcd_contrast.h"

#define WHO_AM_I        0x0F

#define CS              BIT0        // SPI using P1.0 - 1.3 
#define CLK             BIT1        // UCB0 register
#define SIMO            BIT2
#define SOMI            BIT3

#define ACCEL_CONTROL   0x10        // values used to set gyro / accelerometer sensitivity
#define GYRO_CONTROL    0x11

#define ACCEL_G        0.000061f        // values I found online for this sensor
#define GYRO_DPS       0.00875f

// data used to store gyro + accel readings and conversions 
extern volatile int16_t gyro_x, gyro_y, gyro_z;
extern volatile int16_t accel_x, accel_y, accel_z;
extern int32_t average_gyro_x, average_gyro_y, average_gyro_z;
volatile float accel_x_g, accel_y_g, accel_z_g = 0.0f;
volatile float gyro_x_dps, gyro_y_dps, gyro_z_dps = 0.0f;


/**
* some formatting function GPT + VSCode helped me whip up for printing to the LCD screen
* used this in previous projects, just slightly modified it for this project
*/  
void float_to_str_1dp(float value, char *buffer)
{
   

    int i = 0;
    int negative = 0;

    if (value < 0) {
        //buffer[i++] = '-';
        value = -value;
        negative = 1;
        //whole = (int)value;
        //frac = (int)(((value - whole) * 10.0f) + 0.5f);
    } 
    //else { 
        //buffer[i++] = ' ';
    //} 
    
    int whole = (int)value;
    int frac = (int)(((value - whole) * 10.0f) + 0.5f);

    if (frac >= 10) {
        frac = 0;
        whole++;
    }

    // Limit whole to 2 digits max
    if (whole > 99) {
        whole = 99;    
    }    

    if (negative) {
        buffer[i++] = '-';
        if (whole >= 10) {
            buffer[i++] = (whole / 10) + '0';
        }
        buffer[i++] = (whole & 10) + '0';
    } else {
        if (whole >= 10) {
            buffer[i++] = (whole / 10) + '0';
            buffer[i++] = (whole % 10) + '0';
        } else {
            buffer[i++] = ' ';
            buffer[i++] = (whole % 10) + '0';
        }
    }
    buffer[i++] = '.';
    buffer[i++] = frac + '0';
    buffer[i] = '\0';
}


/**
 * Function to write gyroscope values to LCD screen
 * Converted values are already done before calling this function 
 * - to update values, call convert data in main function before calling this
 */
void write_gyro_data(void) {\
    char axis_buffer[20];
    char gyro_buffer[20];
    char accel_buffer[20];
    axis_buffer[0] = '\0';
    gyro_buffer[0] = '\0';
    accel_buffer[0] = '\0';
    append_string(axis_buffer, "     X    Y    Z   ");
    write_line(LINE_1, axis_buffer);

    // Gyro writing
    append_string(gyro_buffer, "dps ");
    char gyro_dps_buffer[8];
    gyro_dps_buffer[0] = '\0';
    float_to_str_1dp(gyro_x_dps, gyro_dps_buffer);
    append_string(gyro_buffer, gyro_dps_buffer);
    append_string(gyro_buffer, " ");
    //gyro_dps_buffer[0] = '\0';
    float_to_str_1dp(gyro_y_dps, gyro_dps_buffer);
    append_string(gyro_buffer, gyro_dps_buffer);
    append_string(gyro_buffer, " ");
    //gyro_dps_buffer[0] = '\0';
    float_to_str_1dp(gyro_z_dps, gyro_dps_buffer);
    append_string(gyro_buffer, gyro_dps_buffer);
    //append_string(gyro_buffer, " ");
    write_line(LINE_2, gyro_buffer);

    // Acceleration writing
    char accel_g_buffer[8];
    accel_g_buffer[0] = '\0';
    append_string(accel_buffer, "G   ");
    float_to_str_1dp(accel_x_g, accel_g_buffer);
    append_string(accel_buffer, accel_g_buffer);
    append_string(accel_buffer, " ");
    //accel_g_buffer[0] = '\0';
    float_to_str_1dp(accel_y_g, accel_g_buffer);
    append_string(accel_buffer, accel_g_buffer);
    append_string(accel_buffer, " ");
    //accel_g_buffer[0] = '\0';
    float_to_str_1dp(accel_z_g, accel_g_buffer);
    append_string(accel_buffer, accel_g_buffer);
    //append_string(accel_buffer, " ");
    write_line(LINE_3, accel_buffer);
    return;
}

/**
 * SPI 4-wire communication to the gyro / accelorometer
 * used to: 
 * - set up the gyro + accel sensitivity
 * - tell the gyro to read + send back values 
 * - returns the UCB0RXBUF values being read in by gyro 
 */
uint8_t spi_write(uint8_t data) {
    while(!(UCB0IFG & UCTXIFG));
    UCB0TXBUF = data;

    while (!(UCB0IFG & UCRXIFG));
    return UCB0RXBUF;
}

/**
 * Converts Gyro + Accelerometer raw readings to meaning full data 
 * Converts Gyro raw --> dps/LSB (Degrees per Second / Least Significant bit on X (roll), Y (pitch), Z (yaw) axis)
 * Converts Acclerometer raw --> G's (force of gravity on X, Y, Z axis)
 */
void convert_data(void) {
    gyro_x_dps = ((gyro_x - average_gyro_x) * GYRO_DPS);
    gyro_y_dps = (((gyro_y - average_gyro_y) * GYRO_DPS) * -1);
    gyro_z_dps = (((gyro_z - average_gyro_z) * GYRO_DPS) * -1);
    accel_x_g = accel_x * ACCEL_G;
    accel_y_g = (accel_y * ACCEL_G); 
    accel_z_g = (accel_z * ACCEL_G);
    return;
}

/**
 * Sends SPI command to Gyro to read from a specific register (gyro + accel) and write back the data from that register
 * - send the register | read_byte  
 * - send 0x00 to latch data being read in do our data buffer
 */
void read_gyro(uint8_t start_register, uint8_t data[12], uint8_t length) {
    int i;
    P1OUT &= ~CS;
    __delay_cycles(10);
    spi_write(start_register | 0x80);
    for (i = 0; i < length; i++) {
        data[i] = spi_write(0x00);
    }
    P1OUT |= CS;
}
/**
 * Config SPI pins + UCB0 register for 4 wire SPI-communication 
 * Pins: 
 * - P1.0 = Chip Select
 * - P1.1 = Clock 
 * - P1.2 = SIMO
 * - P1.3 = SOMI
 */
void spi_init(void) {

    P1DIR |= CS;
    P1OUT |= CS;    // idle high

    P1SEL0 |= (CLK | SIMO | SOMI);      // SPI pins
    P1SEL1 &= ~(CLK | SIMO | SOMI);

    UCB0CTLW0 = UCSWRST;        // put in sw reset
    UCB0CTLW0 |= UCMST; 
    UCB0CTLW0 |= UCSYNC;
    UCB0CTLW0 |= UCMSB;
    UCB0CTLW0 |= UCSSEL__SMCLK;
    UCB0CTLW0 |= UCCKPL;
    UCB0CTLW0 |= UCMODE_0;
    UCB0BRW = 100;
    UCB0CTLW0 &= ~UCSWRST;

    return;
}


/**
 * Debugging function used to see what the Accel + Gyro sensitivities are programmed to
 * - also used to see what the gyros personal register is
 */
//uint8_t spi_read(uint8_t reg) {
   // uint8_t return_value = 0;
    //P1OUT |= CS;
   // __delay_cycles(10);
    //P1OUT &= ~CS;
    //__delay_cycles(10);
    //spi_write(reg | 0x80);
    //return_value = spi_write(0x00);
   // P1OUT |= CS;
    //__delay_cycles(10);
    //return return_value;
//}


/**
 * Since the LSM6DS33 Gyro / Accelerometer is initially in a sleep mode when powered on 
 * we need to config the gyro and accel the their respective values 
 */
void imu_init(void) {
    //uint8_t whoiam = 0;

    //whoiam = spi_read(WHO_AM_I);
    P1OUT &= ~CS;
    __delay_cycles(10);
    spi_write(ACCEL_CONTROL & 0x7F);        // config the acclerometer
    spi_write(0x40);
    P1OUT |= CS;
    __delay_cycles(10);
    P1OUT &= ~CS;
    __delay_cycles(10);
    spi_write(GYRO_CONTROL & 0x7F);           // config the gyroscope
    spi_write(0x40);
    P1OUT |= CS;
    __delay_cycles(10);

    return;
}