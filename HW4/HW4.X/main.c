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
void i2c_master_setup();
void initExpander();

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
    i2c_master_setup();
    initExpander();
    
    while(1){
        _CP0_SET_COUNT(0);
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
        if ((getExpander() & 0b10000000) == 0b10000000) {
            setExpander(0b00000001,1);
        }
        else {
            setExpander(0b00000001,0);                            
        }
    }
}

void initSPI1(){
    TRISBbits.TRISB7 = 0b0;
    CS = 1;
    SS1Rbits.SS1R = 0b0100;
    SDI1Rbits.SDI1R = 0b0000;
    RPB8Rbits.RPB8R = 0b0011;
    ANSELBbits.ANSB14 = 0;
    
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
    if(channel == 0) { // VoutA
        CS = 0;
        SPI1_IO((voltage >> 4) | 0b01110000);
        SPI1_IO(voltage << 4);
        CS = 1;
    }
    if(channel == 1) { // VoutB
        CS = 0;
        SPI1_IO((voltage >> 4) | 0b11110000);
        SPI1_IO(voltage << 4);
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

void i2c_master_start(void) {
    I2C2CONbits.SEN = 1;
    while(I2C2CONbits.SEN) {
        ;
    }
}

void i2c_master_restart(void) {
    I2C2CONbits.RSEN = 1;
    while(I2C2CONbits.RSEN) {
        ;
    }
}

void i2c_master_send(unsigned char byte) {
    I2C2TRN = byte;
    while(I2C2STATbits.TRSTAT) {
        ;
    }
    if(I2C2STATbits.ACKSTAT) {
    }
}

unsigned char i2c_master_recv(void) {
    I2C2CONbits.RCEN = 1;
    while(!I2C2STATbits.RBF) {
        ;
    }
    return I2C2RCV;
}

void i2c_master_ack(int val) {
    I2C2CONbits.ACKDT = val;
    I2C2CONbits.ACKEN = 1;
    while(I2C2CONbits.ACKEN) {
        ;
    }
}

void i2c_master_stop(void) {
    I2C2CONbits.PEN = 1;
    while(I2C2CONbits.PEN) {
        ;
    }
}

void initExpander() {
    
    i2c_master_start();
    i2c_master_send(0b01000000);
    i2c_master_send(0x0A);
    i2c_master_send(0b00000000);
    i2c_master_stop();
    
}


void setExpander(char pin, char level) {
    char levelbyte = 0b11111111;
    if (level == 0) {
        levelbyte = 0b00000000;
    }
    i2c_master_start();
    i2c_master_send(0b01000000);
    i2c_master_send(0x0A);
    i2c_master_send(pin & levelbyte);
    i2c_master_stop();
}

void getExpander() {
    i2c_master_start();
    i2c_master_send(0b01000000);
    i2c_master_send(0x09);
    i2c_master_restart();
    i2c_master_send(0b01000001);
    char result = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    return result;
}


void i2c_master_setup(void) {
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;
    I2C2BRG = 233;
    I2C2CONbits.ON = 1;
    i2c_master_start();
    i2c_master_send(0b01000000);
    i2c_master_send(0x00);
    i2c_master_send(0b11110000);
    i2c_master_stop();
}