#ifndef PIC32_CONFIG_H
#define PIC32_CONFIG_H

#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include "i2c.h"
#include "spi.h"
#include <math.h>


// DEVCFG0
#pragma config DEBUG 		= 0b10	// no debugging
#pragma config JTAGEN 		= 0b0 	// no jtag
#pragma config ICESEL 		= 0b11 	// use PGED1 and PGEC1
#pragma config PWP 			= 0x3F  // no write protect
#pragma config BWP 			= 0b1 	// no boot write protect
#pragma config CP 			= 0b1 	// no code protect

// DEVCFG1
#pragma config FNOSC 		= 0b011	// use primary oscillator with pll
#pragma config FSOSCEN 		= 0b0 	// turn off secondary oscillator
#pragma config IESO 		= 0b0 	// no switching clocks
#pragma config POSCMOD 		= 0b10 	// high speed crystal mode
#pragma config OSCIOFNC 	= 0b1 	// free up secondary osc pins
#pragma config FPBDIV 		= 0b00 	// divide CPU freq by 1 for peripheral bus clock
#pragma config FCKSM 		= 0b11 	// do not enable clock switch
#pragma config WDTPS 		= 0b10100 	// slowest wdt
#pragma config WINDIS 		= 0b1 	// no wdt window
#pragma config FWDTEN 		= 0b0 	// wdt off by default
#pragma config FWDTWINSZ 	= 0b11 	// wdt window at 25%

// DEVCFG2 - get the CPU clock to 48MHz
#pragma config FPLLIDIV 	= 0b001	// divide input clock to be in range 4-5MHz
#pragma config FPLLMUL 		= 0b111	// multiply clock after FPLLIDIV
#pragma config FPLLODIV 	= 0b001	// divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV 	= 0b001	// divider for the 8MHz input clock, then multiply by 12 to get 48MHz for USB
#pragma config UPLLEN 		= 0b0 	// USB clock on

// DEVCFG3
#pragma config USERID 		= 0b0 	// some 16bit userid, doesn't matter what
#pragma config PMDL1WAY 	= 0b0 	// allow multiple reconfigurations
#pragma config IOL1WAY 		= 0b0 	// allow multiple reconfigurations
#pragma config FUSBIDIO 	= 0b1 	// USB pins controlled by USB module
#pragma config FVBUSONIO 	= 0b1 	// USB BUSON controlled by USB module

#endif
#define  LEVELS     (int)       256
#define  PI         (double)    3.14159265
#define  I2C_FREQ   (int)       12000000
#define  PIC_FREQ   (int)       48000000
#define  A_FREQ     (int)       10
#define  B_FREQ     (int)       5
#define  A_UPDATER  (int)       PIC_FREQ/A_FREQ/LEVELS
#define  B_UPDATER  (int)       PIC_FREQ/B_FREQ/LEVELS


int main()
{
    int             Acount = 0, i = 0;
    unsigned char   Alevel = 0, Blevel = 0;
    unsigned char   sine[A_UPDATER];
    for (;i<A_UPDATER;i++)
    {
        sine[i] = ((LEVELS-1)/2*sin(((double)(2*PI*Acount))/((double)LEVELS)*2))+(LEVELS/2);
        Acount++;
    }
    Acount = 0;
    // PIC32 Setup
    __builtin_disable_interrupts();
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);
    BMXCONbits.BMXWSDRM = 0x0;  // 0 data RAM access wait states
    INTCONbits.MVEC = 0x1;      // enable multi vector interrupts
    DDPCONbits.JTAGEN = 0;      // disable JTAG to get pins back
    TRISAbits.TRISA4 = 0;       // RA4 is output
    TRISBbits.TRISB4 = 1;       // RB4 is input
    LATAbits.LATA4 = 0;			// LED is off
    WDTCONbits.ON = 0;
    initExpander(GP7);          // Make GP7 (button) input, rest outputs
    initSPI1();
    __builtin_enable_interrupts();
    
    while(1)
    {
        _CP0_SET_COUNT(0);
        setVoltage(A,Alevel);
        setVoltage(B,Blevel);
        Alevel = sine[Acount];
        Acount++;
        Blevel++;
        //setExpander(GP0,(char)((getExpander()&GP7)==GP7));  //Set LED on/off
        while(_CP0_GET_COUNT()<(B_UPDATER/2)){;}
    }   
}