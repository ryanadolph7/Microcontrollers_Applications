#include <msp430fr2310.h>
#include <stdbool.h>
#include <stdint.h>

volatile unsigned int ADC_Value = 0;
volatile uint8_t speedOfPatterns = 0;
volatile uint16_t tick10ms = 0;

void init_adc(void){

    //set pin for adc
    P1SEL1 |= BIT2;
    P1SEL0 |= BIT2;

    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCON;

    ADCCTL1 |= ADCSSEL_2;
    ADCCTL1 |= ADCSHP;

    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;

    ADCMCTL0|= ADCINCH_2;

    ADCIE |= ADCIE0;

    ADCCTL0 |= ADCENC;      // enable conversions
}

void timer_b1_10ms_init(void){

    TB1CTL |= TBCLR;
    TB1CTL |= TBSSEL__ACLK;
    TB1CTL |= MC__UP;
    TB1CTL |= TBCLR;
    TB1CCR0 = 327;
    TB1CCTL0 = CCIE;
}

void potentiometer_init(void){
    init_adc();
    timer_b1_10ms_init();
}

#pragma vector = TIMER1_B0_VECTOR
__interrupt void TB1_CCR0_ISR(void){

    ADCCTL0 |= ADCSC;       //start ADC conversion

    tick10ms++;

    TB1CCTL0 &= ~CCIFG;


}


#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void){


    ADC_Value = ADCMEM0;


    if (ADC_Value > 3900){
        //make led bar blink at a certain speed;
        speedOfPatterns = 15;
    }
    else if(ADC_Value > 3650){
        speedOfPatterns = 14;
    }
    else if(ADC_Value > 3400){
        speedOfPatterns = 13;
    }
    else if(ADC_Value > 3150){
        speedOfPatterns = 12;
    }
    else if(ADC_Value > 2900){
        speedOfPatterns = 11;
    }
    else if(ADC_Value > 2650){
        speedOfPatterns = 10;
    }
    else if(ADC_Value > 2400){
        speedOfPatterns = 9;
    }
    else if(ADC_Value > 2150){
        speedOfPatterns = 8;
    }
    else if(ADC_Value > 1900){
        speedOfPatterns = 7;
    }
    else if(ADC_Value > 1650){
        speedOfPatterns = 6;
    }
    else if(ADC_Value > 1400){
        speedOfPatterns = 5;
    }
    else if(ADC_Value > 1150){
        speedOfPatterns = 4;
    }
    else if(ADC_Value > 900){
        speedOfPatterns = 3;
    }
    else if(ADC_Value > 650){
        speedOfPatterns = 2;
    }
    else if(ADC_Value > 400){
        speedOfPatterns = 1;
    }
    else{
        speedOfPatterns = 0;
    }

    ADCIFG &= ~ADCIFG0;

}







