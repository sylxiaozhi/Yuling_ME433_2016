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

#define CS LATBbits.LATB8       // chip select pin

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
    TRISAbits.TRISA4 = 0;
    TRISBbits.TRISB4 = 1;
    
    __builtin_enable_interrupts();
    
    
    // send a byte via spi and return the response
    unsigned char spi_io(unsigned char o) {
        SPI4BUF = o;
        while(!SPI4STATbits.SPIRBF) { // wait to receive the byte
            ;
        }
        return SPI4BUF;
    }
    
    // initialize spi4 and the ram module
    void ram_init() {
        // set up the chip select pin as an output
        // the chip select pin is used by the sram to indicate
        // when a command is beginning (clear CS to low) and when it
        // is ending (set CS high)
        TRISBbits.TRISB8 = 0;
        CS = 1;
        
        // Master - SPI4, pins are: SDI4(F4), SDO4(F5), SCK4(F13).
        // we manually control SS4 as a digital output (F12)
        // since the pic is just starting, we know that spi is off. We rely on defaults here
        
        // setup spi4
        SPI4CON = 0;              // turn off the spi module and reset it
        SPI4BUF;                  // clear the rx buffer by reading from it
        SPI4BRG = 0x3;            // baud rate to 10 MHz [SPI4BRG = (80000000/(2*desired))-1]
        SPI4STATbits.SPIROV = 0;  // clear the overflow bit
        SPI4CONbits.CKE = 1;      // data changes when clock goes from hi to lo (since CKP is 0)
        SPI4CONbits.MSTEN = 1;    // master operation
        SPI4CONbits.ON = 1;       // turn on spi 4
        
        // send a ram set status command.
        CS = 0;                   // enable the ram
        spi_io(0x01);             // ram write status
        spi_io(0x41);             // sequential mode (mode = 0b01), hold disabled (hold = 0)
        CS = 1;                   // finish the command
    }
    
    // write len bytes to the ram, starting at the address addr
    void ram_write(unsigned short addr, const char data[], int len) {
        int i = 0;
        CS = 0;                        // enable the ram by lowering the chip select line
        spi_io(0x2);                   // sequential write operation
        spi_io((addr & 0xFF00) >> 8 ); // most significant byte of address
        spi_io(addr & 0x00FF);         // the least significant address byte
        for(i = 0; i < len; ++i) {
            spi_io(data[i]);
        }
        CS = 1;                        // raise the chip select line, ending communication
    }
    
    // read len bytes from ram, starting at the address addr
    void ram_read(unsigned short addr, char data[], int len) {
        int i = 0;
        CS = 0;
        spi_io(0x3);                   // ram read operation
        spi_io((addr & 0xFF00) >> 8);  // most significant address byte
        spi_io(addr & 0x00FF);         // least significant address byte
        for(i = 0; i < len; ++i) {
            data[i] = spi_io(0);         // read in the data
        }
        CS = 1;
    }
    
    int main(void) {
        unsigned short addr1 = 0x1234;                  // the address for writing the ram
        char data[] = "Help, I'm stuck in the RAM!";    // the test message
        char read[] = "***************************";    // buffer for reading from ram
        char buf[100];                                  // buffer for comm. with the user
        unsigned char status;                           // used to verify we set the status
        NU32_Startup();   // cache on, interrupts on, LED/button init, UART init
        ram_init();
        
        // check the ram status
        CS = 0;
        spi_io(0x5);                                      // ram read status command
        status = spi_io(0);                               // the actual status
        CS = 1;
        
        sprintf(buf, "Status 0x%x\r\n",status);
        NU32_WriteUART3(buf);
        
        sprintf(buf,"Writing \"%s\" to ram at address 0x%x\r\n", data, addr1);
        NU32_WriteUART3(buf);
        // write the data to the ram
        ram_write(addr1, data, strlen(data) + 1);         // +1, to send the '\0' character
        ram_read(addr1, read, strlen(data) + 1);          // read the data back
        sprintf(buf,"Read \"%s\" from ram at address 0x%x\r\n", read, addr1);
        NU32_WriteUART3(buf);
        
        while(1) {
            ;
        }
        return 0;
    }
}