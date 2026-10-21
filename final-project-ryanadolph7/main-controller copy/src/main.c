#include <msp430fr2153.h>
#include <stdbool.h>
#include <stdint.h>
#include "gyro_controller.h"
#include "lcd_contrast.h"
#include "distance_sensor.h"


#define READ_REG        0x22


uint8_t data[12];
volatile int16_t gyro_x, gyro_y, gyro_z;
volatile int16_t accel_x, accel_y, accel_z;
volatile uint8_t who = 0;
volatile uint8_t accel_check, gyro_check = 0;
int32_t average_gyro_x, average_gyro_y, average_gyro_z = 0;

int main(void)
{
    // Stop the watchdog timer
    WDTCTL = WDTPW | WDTHOLD;
    
    // Disable low-power mode
    PM5CTL0 &= ~LOCKLPM5;

    spi_init();
    timer_init();
    lcd_data_init();
    lcd_init();
    imu_init();
    distance_sensor_init();

    __enable_interrupt();

    // Debugging commands to the gyroscope and the LCD screen
    //accel_check = spi_read(0x10);
    //gyro_check = spi_read(0x11);
    //lcd_command(0x80);
    //lcd_data('H');
    //lcd_data('e');
    //lcd_data('l');
    //lcd_data('l');
    //lcd_data('o');
    int i = 0;              // a small filter / average values taken for the first 500 readings to try to average out the the readings 
                            // the gyro intial readings were x = 4.4-4.9, y = 5.1-5.7, z = 3.6-3.8 so i wanted to get these values down to a normal range
    for (i; i < 500; i++) {
        read_gyro(READ_REG, data, 12);
        int16_t gx = (int16_t)((data[1] <<8) | data[0]);
        int16_t gy = (int16_t)((data[3] <<8) | data[2]);
        int16_t gz = (int16_t)((data[5] <<8) | data[4]);
        average_gyro_x += gx;
        average_gyro_y += gy;
        average_gyro_z += gz;
        delay_timer_ms(5);
    }
    average_gyro_x = average_gyro_x / 500;
    average_gyro_y = average_gyro_y / 500;
    average_gyro_z = average_gyro_z / 500;
    
    while(true)
    {
        //who = spi_read(0x0F);
        //__delay_cycles(50000);

        read_gyro(READ_REG, data, 12);
        gyro_x = (int16_t)(data[1] << 8) | data[0];         // need to add filtering to get these to be stable values  
        gyro_y = (int16_t)(data[3] << 8) | data[2];         // and not changing so large when running
        gyro_z = (int16_t)(data[5] << 8) | data[4];         // should add a Kalman filter
        accel_x = (int16_t)(data[7] << 8) | data[6];        // roll
        accel_y = (int16_t)(data[9] << 8) | data[8];        // pitch 
        accel_z = (int16_t)(data[11] << 8) | data[10];      // yaw
        
        convert_data();
        write_gyro_data();
        write_distance_data();
        delay_timer_ms(50);     // small delay after reading the Gyro + distance sensor + outputting values
       
    }
}
