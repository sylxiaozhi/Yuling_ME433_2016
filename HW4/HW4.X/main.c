#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<math.h>

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
#pragma config OSCIOFNC = OFF  // free up secondary osc pins
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
#pragma config USERID = 0x1234 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module

#define CS LATBbits.LATB7 // chip select pin
#define SineCount 100
#define TriCount 200
#define PI 3.14159

static volatile float SineWaveform[SineCount];   // sine waveform
static volatile float TriWaveform[TriCount];   // triangle waveform
unsigned char read  = 0x00;
unsigned char checkGP7 = 0x00;
char SPI1_IO(char write);
void initSPI1();
void setVoltage(char channel, char voltage);
void MakeSinWave();
void MakeTriWave();

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
    TRISAbits.TRISA4 = 0;   //RA4 (PIN#12) for Green LED
    LATAbits.LATA4 = 1;
    TRISBbits.TRISB4 = 1;   //RB4 (PIN#11) for pushbutton
    
    __builtin_enable_interrupts();
    
    MakeSinWave();
    MakeTriWave();
    initSPI1();
     
       
    while(1){
        _CP0_SET_COUNT(0);
        LATAINV = 0x10; // make sure timer2 works
        static int count1 = 0;
        static int count2 = 0;
        setVoltage(0,SineWaveform[count1]); 
        setVoltage(1,TriWaveform[count2]);
        count1++;
        count2++;
        if(count1 >= SineCount){
            count1 = 0;
        }
        if(count2 >= TriCount){
            count2 = 0;
        }
        while(_CP0_GET_COUNT() < 24000) { 
            ;
        }
    }
}

void initSPI1(){
    // set up the chip select pin as an output
    // the chip select pin is used by the MCP4902DAC to indicate
    // when a command is beginning (clear CS to low) and when it
    // is ending (set CS high)
    TRISBbits.TRISB7 = 0b0;
    CS = 1;
    SS1Rbits.SS1R = 0b0100;   // assign SS1 to RB7
    SDI1Rbits.SDI1R = 0b0000; // assign SDI1 to RA1 
    RPB8Rbits.RPB8R = 0b0011; // assign SDO1 to RB8
    ANSELBbits.ANSB14 = 0;    // turn off AN10
    
    // setup SPI1
    SPI1CON = 0;              // turn off the SPI1 module and reset it
    SPI1BUF;                  // clear the rx buffer by reading from it
    SPI1BRG = 0x1;            // baud rate to 12 MHz [SPI4BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0;  // clear the overflow bit
    SPI1CONbits.MODE32 = 0;   // use 8 bit mode
    SPI1CONbits.MODE16 = 0;
    SPI1CONbits.CKE = 1;      // data changes when clock goes from hi to lo (since CKP is 0)
    SPI1CONbits.MSTEN = 1;    // master operation
    SPI1CONbits.ON = 1;       // turn on SPI 1
}

char SPI1_IO(char write){
    SPI1BUF = write;
    while(!SPI1STATbits.SPIRBF) { // wait to receive the byte
     ;
     }
    return SPI1BUF; 
}

void setVoltage(char channel, char voltage){
    if(channel == 0) { // 0 for VoutA
        CS = 0; 
        SPI1_IO((voltage >> 4) | 0b01110000); // 4 configuration bits
        SPI1_IO(voltage << 4); // Data bits 
        CS = 1;   
    }
    if(channel == 1) { // 1 for VoutB
        CS = 0; 
        SPI1_IO((voltage >> 4) | 0b11110000); // 4 configuration bits
        SPI1_IO(voltage << 4); // Data bits
        CS = 1;   
    }
}

void MakeSinWave(){
    int i;
    for(i = 0; i < SineCount; i++){
        SineWaveform[i] = 127+128*sin(2*PI*i*0.01);
        }
}


void MakeTriWave(){
    int j;
    for(j = 0; j < TriCount; j++){
        TriWaveform[j] = 255*(j*0.005); 
    }
}

