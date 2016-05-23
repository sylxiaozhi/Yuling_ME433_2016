#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <math.h>
#include "i2c_master_noint.h"

// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // free up secondary osc pins
#pragma config FPBDIV = DIV_1 // divide CPU freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576 // slowest wdt
#pragma config WINDIS = OFF // no wdt window
#pragma config FWDTEN = OFF // wdt off by default
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the CPU clock to 48MHz
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiply by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module

#define CS LATBbits.LATB7  // chip select pin
#define PI 3.14159265
#define SLAVE_ADDR 0x6B

//the read data of temperature, gyroscope and accelerator
static signed short temp;
static signed short x_angular_rate;
static signed short y_angular_rate;
static signed short z_angular_rate;
static signed short x_linear_accel;
static signed short y_linear_accel;
static signed short z_linear_accel;


void __ISR(_TIMER_2_VECTOR, IPL5SOFT) PWMcontroller(void) { // step 1: the ISR
    
    //OC1RS = 1500;
    OC1RS = 1500+1500*(x_linear_accel/32768.0);
    OC2RS = 1500+1500*(y_linear_accel/32768.0);
    
    IFS0bits.T2IF = 0;
}

void init_accele(){
    i2c_master_start();
    i2c_master_send(0xD6);
    i2c_master_send(0x10);
    i2c_master_send(0x80);
    i2c_master_stop();
}

void init_gyro(){
    i2c_master_start();
    i2c_master_send(0xD6);
    i2c_master_send(0x11);
    i2c_master_send(0x80);
    i2c_master_stop();
}

void I2C_read_multiple(char address, char regis, unsigned char * data, char length){
    i2c_master_start();
    i2c_master_send(address << 1);
    i2c_master_send(regis);
    i2c_master_restart();
    i2c_master_send(address << 1 | 1);
    int i;
    for(i=0;i<length-1;i++) {
        data[i]= i2c_master_recv();
        i2c_master_ack(0);
    }
    data[length-1]= i2c_master_recv();
    i2c_master_ack(1);
}


int main() {
    
    __builtin_disable_interrupts();
    
    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);
    
    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;
    
    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;
    
    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;
    
    // do your TRIS and LAT commands here
    TRISAbits.TRISA4=0;
    TRISBbits.TRISB4=1;
    
    //spi_init();
    i2c_master_setup();
    //initExpander() ;
    init_accele();
    init_gyro();
    
    // for Timer2
    PR2 = 2999;                   // period = (PR2+1) * N * 20.8 ns = 0.001 s, 1 kHz
    TMR2 = 0;                     // initial TMR2 count is 0
    T2CONbits.TCKPS = 0b100;      // Timer2 prescaler N=16 (1:16)
    T2CONbits.ON = 1;             // turn on Timer2
    
    OC1CONbits.OCM = 0b110;       // PWM mode without fault pin; other OC1CON bits are defaults
    OC1CONbits.ON = 1;            // turn on OC1
    OC1CONbits.OC32 = 0;
    OC1CONbits.OCTSEL = 0;        // select Timer2
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> newbranch
    
    OC2CONbits.OCM = 0b110;       // PWM mode without fault pin; other OC1CON bits are defaults
    OC2CONbits.ON = 1;            // turn on OC2
    OC2CONbits.OC32 = 0;
    OC2CONbits.OCTSEL = 0;        // select Timer2
    
    IPC2bits.T2IP = 5;            // step 4: interrupt priority
    IPC2bits.T2IS = 0;            // step 4: interrupt priority
    IFS0bits.T2IF = 0;            // step 5: clear the int flag
    IEC0bits.T2IE = 1;            // step 6: enable Timer2 by setting IEC0<11>
    
    RPB15Rbits.RPB15R = 0b0101; // assign OC1 to RB15
    RPA1Rbits.RPA1R = 0b0101; // assign OC2 to RA1
    
    __builtin_enable_interrupts();
    
    
    LATAbits.LATA4=0;
    
    
    while(1) {
<<<<<<< HEAD
=======
=======
    
    OC2CONbits.OCM = 0b110;       // PWM mode without fault pin; other OC1CON bits are defaults
    OC2CONbits.ON = 1;            // turn on OC2
    OC2CONbits.OC32 = 0;
    OC2CONbits.OCTSEL = 0;        // select Timer2
    
    IPC2bits.T2IP = 5;            // step 4: interrupt priority
    IPC2bits.T2IS = 0;            // step 4: interrupt priority
    IFS0bits.T2IF = 0;            // step 5: clear the int flag
    IEC0bits.T2IE = 1;            // step 6: enable Timer2 by setting IEC0<11>
    
    RPB15Rbits.RPB15R = 0b0101; // assign OC1 to RB15
    RPA1Rbits.RPA1R = 0b0101; // assign OC2 to RA1
    
    __builtin_enable_interrupts();
    
    
    LATAbits.LATA4=0;
    
    //01101011 6B
    
    while(1) {
        
        // intial test for the WHO AM I
        /*
         i2c_master_start();
         i2c_master_send(0xD6); //0b11010110
         01101011
         i2c_master_send(0x0F);
         i2c_master_restart();
         i2c_master_send(0xD7); //0b11010111
         unsigned char r;
         r = i2c_master_recv();
         i2c_master_ack(1);
         i2c_master_stop();
         */
        /*
         I2C_read_multiple(0x6B,0x0F,&r,1);
         _CP0_SET_COUNT(0);
         while(_CP0_GET_COUNT() < 12000) {
         ;
         }
         
         //0b01101001
         if (r==0x69) {
         LATAbits.LATA4=1;
         }
         */
>>>>>>> Yuling
>>>>>>> newbranch
        unsigned char r;
        unsigned char data[20];
        //unsigned char ddx_high;
        //unsigned char ddx_low;
        //unsigned char r;
        I2C_read_multiple(0x6B,0x0F,&r,1);
        _CP0_SET_COUNT(0);
        //I2C_read_multiple(0x6B,0x28,&ddx_low,1);
        //I2C_read_multiple(0x6B,0x29,&ddx_high,1);
        while(_CP0_GET_COUNT() < 120000) { // 50 Hz
            ;
<<<<<<< HEAD
        }
        if (r==0x69) {
            LATAbits.LATA4=1;
        }
        
        I2C_read_multiple(0x6B,0x20,data,14);
        
        while(_CP0_GET_COUNT() < 240000) { // 50 Hz
            ;
        }
=======
        }
        if (r==0x69) {
            LATAbits.LATA4=1;
        }
        
        I2C_read_multiple(0x6B,0x20,data,14);
        
        while(_CP0_GET_COUNT() < 240000) { // 50 Hz
            ;
        }
>>>>>>> newbranch
        temp=data[1]<<8|data[0];
        x_angular_rate=data[3]<<8|data[2];
        y_angular_rate=data[5]<<8|data[4];
        z_angular_rate=data[7]<<8|data[6];
        x_linear_accel=data[9]<<8|data[8];
        y_linear_accel=data[11]<<8|data[10];
        z_linear_accel=data[13]<<8|data[12];
        
<<<<<<< HEAD

=======
<<<<<<< HEAD

=======
        
        //x_linear_accel=ddx_high<<8|ddx_low;
        //y_linear_accel=r;         
        
        
>>>>>>> Yuling
>>>>>>> newbranch
    }
    
    
}