/*
 * File:   main.c
 * 
 *
 * Created on 03 September 2016, 19:34
 */


#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#define IO_LEDRED LATA1
#define IO_GREENLED LATA0
#define IO_LOCK PORTAbits.RA4
#define IO_FAN LATA5

#define _XTAL_FREQ  500000
#define TMR0_INTERRUPT_TICKER_FACTOR    2

#pragma config FOSC = INTOSC    // Oscillator Selection->INTOSC oscillator: I/O function on CLKIN pin
#pragma config WDTE = OFF    // Watchdog Timer Enable->WDT disabled
#pragma config PWRTE = OFF    // Power-up Timer Enable->PWRT disabled
#pragma config MCLRE = OFF    // MCLR Pin Function Select->MCLR/VPP pin function is digital input
#pragma config CP = OFF    // Flash Program Memory Code Protection->Program memory code protection is disabled
#pragma config CPD = OFF    // Data Memory Code Protection->Data memory code protection is disabled
#pragma config BOREN = ON    // Brown-out Reset Enable->Brown-out Reset enabled
#pragma config CLKOUTEN = OFF    // Clock Out Enable->CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin
#pragma config IESO = ON    // Internal/External Switchover->Internal/External Switchover mode is enabled
#pragma config FCMEN = ON    // Fail-Safe Clock Monitor Enable->Fail-Safe Clock Monitor is enabled

// CONFIG2
#pragma config WRT = OFF    // Flash Memory Self-Write Protection->Write protection off
#pragma config PLLEN = ON    // PLL Enable->4x PLL enabled
#pragma config STVREN = ON    // Stack Overflow/Underflow Reset Enable->Stack Overflow or Underflow will cause a Reset
#pragma config BORV = LO    // Brown-out Reset Voltage Selection->Brown-out Reset Voltage (Vbor), low trip point selected.
#pragma config LVP = ON    // Low-Voltage Programming Enable->Low-voltage programming enabled



// variables

uint8_t OverTemp = 0;
uint16_t convertedValue = 0;
uint8_t pwmcount = 0;
uint8_t pwmduty = 0;
uint16_t CountCallBack = 0;

void Timer0_CallBack(void) {
    // Get the ADC voltage

    // Start the conversion
    ADCON0bits.GO_nDONE = 1;

    // Wait for the conversion to finish
    while (ADCON0bits.GO_nDONE) {
    }
    // Conversion finished, get the result
    convertedValue = ((ADRESH << 8) + ADRESL);

    // compare the value against pre-calculated temperature values and set the fan speed accordingly

    if (convertedValue > 250) {// temperature has exceeded 70C.  Over temperature event has occurred
        OverTemp = 1;
        pwmduty = 64; // set fan speed to 100%
    } else {
        OverTemp = 0;
        if (convertedValue > 240) {
            pwmduty = 64;
        }// temperature has exceeded 65C 
        else if (convertedValue > 229) {
            pwmduty = 60;
        }// temperature has exceeded 60C            
        else if (convertedValue > 219) {
            pwmduty = 55;
        }// temperature has exceeded 55C 
        else if (convertedValue > 208) {
            pwmduty = 50;
        }// temperature has exceeded 50C 
        else if (convertedValue > 198) {
            pwmduty = 45;
        }// temperature has exceeded 45C 
        else if (convertedValue > 188) {
            pwmduty = 40;
        }// temperature has exceeded 40C 
        else if (convertedValue > 177) {
            pwmduty = 35;
        }// temperature has exceeded 35C 
        else if (convertedValue > 167) {
            pwmduty = 30;
        }// temperature has exceeded 30C 
        else {
            pwmduty = 0;
        } // temperature is less than 30C             
    }


    if (OverTemp == 1) { // Over temperature event has occurred so toggle the red led
        IO_GREENLED = 0;
        if (IO_LEDRED == 1) {
            IO_LEDRED = 0;
        } else {
            IO_LEDRED = 1;
        }

    } else {
        // get the status of the lock input
        if (IO_LOCK == 1) { // not locked
            IO_GREENLED = 0;
            IO_LEDRED = 1;
        } else { // locked
            IO_GREENLED = 1;
            IO_LEDRED = 0;
        }
    }



}

void interrupt tc_int(void) // interrupt function 
 {
    if (INTCONbits.T0IF && INTCONbits.T0IE) // TMR0 interrupt has triggered
    {
        TMR0 = 12; // reset TMR0 value

        // callback function - called every 2th pass
        if (++CountCallBack >= TMR0_INTERRUPT_TICKER_FACTOR) {
            // ticker function call
            Timer0_CallBack();

            // reset ticker counter
            CountCallBack = 0;
        }

        // Clear the TMR0 interrupt flag
        INTCONbits.TMR0IF = 0;
    }
    else if (PIR1bits.TMR2IF && PIE1bits.TMR2IE) // TMR2 interrupt has fired
    {
        // A basic PWM function that sets a pulse width of 0 to 64
        pwmcount = pwmcount + 1;

        if (pwmcount < pwmduty) {
            IO_FAN = 1;
        } else {
            IO_FAN = 0;
        }

        if (pwmcount >= 64) {
            pwmcount = 0;
        }

        PIR1bits.TMR2IF = 0;
        TMR2 = 0x00;
    }

}

void main(void) {

    // Set up the oscillator

    // SCS FOSC; SPLLEN disabled; IRCF 500KHz_HF; 
    OSCCON = 0x50;
    // LFIOFR disabled; HFIOFL not stable; OSTS intosc; PLLR disabled; HFIOFS not stable; HFIOFR disabled; MFIOFR disabled; T1OSCR disabled; 
    OSCSTAT = 0x00;
    // TUN 0; 
    OSCTUNE = 0x00;

    // Set pin direction and analogue inputs
    LATA = 0x0;
    ANSELA = 0x4;
    WPUA = 0x3F;
    TRISA = 0x1C;

    OPTION_REGbits.nWPUEN = 0x0;
    APFCON = 0x01;

    IO_FAN = 0;

    // initialize the ADC 

    // GO_nDONE stop; ADON enabled; CHS AN0; 
    ADCON0 = 0x01;
    // ADFM right; ADPREF VDD; ADCS FOSC/2; 
    ADCON1 = 0x80;

    ADRESL = 0x00;
    ADRESH = 0x00;
    // select the A/D channel
    ADCON0bits.CHS = 2;
    // Turn on the ADC module
    ADCON0bits.ADON = 1;

    //Set TMR2 to 1KHz for the PWM output

    // T2CKPS 1:1; T2OUTPS 1:1; TMR2ON on; 
    T2CON = 0b00000000;
    // PR2 12; 
    PR2 = 0x0C;
    // TMR2 0; 
    TMR2 = 0x00;
    // Start the Timer by writing to TMRxON bit
    T2CONbits.TMR2ON = 1;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;

    // initialize TMR0 to run once a second    
    OPTION_REG = 0x87;
    TMR0 = 12;
    INTCON = 0xA0;

    // Clear Interrupt flag before enabling the interrupt
    INTCONbits.TMR0IF = 0;
    // Enabling TMR0 interrupt
    INTCONbits.TMR0IE = 1;


    // Enable Interrupts

    INTCONbits.T0IE = 1; // Enable interrupt on TMR0 overflow
    OPTION_REGbits.INTEDG = 0; // falling edge trigger the interrupt
    INTCONbits.INTE = 1; // enable the external interrupt
    INTCONbits. GIE = 1; // Global interrupt enable
    INTCONbits.PEIE = 1;


    while (1) {
    }

    return;

}
