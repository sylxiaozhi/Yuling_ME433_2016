#include <xc.h>
#include "i2c_master.h"

char i2c_master_read(char reg_addr)
{
    char read_master;
    i2c_master_start();
    i2c_master_send((SLAVE_ADDR << 1) | 0); // writing
    i2c_master_send(reg_addr);
    i2c_master_restart();                   // send a RESTART to read
    i2c_master_send((SLAVE_ADDR << 1) | 1); // reading
    read_master = i2c_master_recv();        // receive a byte from the bus
    i2c_master_ack(1);
    i2c_master_stop();
    return read_master;
}

void i2c_master_write(char reg_addr, char byte)
{
    i2c_master_start();
    i2c_master_send((SLAVE_ADDR << 1) | 0); // writing
    i2c_master_send(reg_addr);
    i2c_master_send(byte);
    i2c_master_stop();
}

void i2c_master_setup(void) {
  I2C2BRG = 233; // for 100kHz;     // I2CBRG = [1/(2*Fsck) - PGD]*Pblck - 2 
  I2C2CONbits.ON = 1;               // turn on the I2C2 module
}

// Start a transmission on the I2C bus
void i2c_master_start(void) {
    I2C2CONbits.SEN = 1;            // send the start bit
    while(I2C2CONbits.SEN) { ; }    // wait for the start bit to be sent
}

void i2c_master_restart(void) {     
    I2C2CONbits.RSEN = 1;           // send a restart 
    while(I2C2CONbits.RSEN) { ; }   // wait for the restart to clear
}

void i2c_master_send(unsigned char byte) { // send a byte to slave
  I2C2TRN = byte;                   // if an address, bit 0 = 0 for write, 1 for read
  while(I2C2STATbits.TRSTAT) { ; }  // wait for the transmission to finish
  while(I2C2STATbits.ACKSTAT) {;}
}

unsigned char i2c_master_recv(void) { // receive a byte from the slave
    I2C2CONbits.RCEN = 1;             // start receiving data
    while(!I2C2STATbits.RBF) { ; }    // wait to receive the data
    return I2C2RCV;                   // read and return the data
}

void i2c_master_ack(int val) {        // sends ACK = 0 (slave should send another byte)
                                      // or NACK = 1 (no more bytes requested from slave)
    I2C2CONbits.ACKDT = val;          // store ACK/NACK in ACKDT
    I2C2CONbits.ACKEN = 1;            // send ACKDT
    while(I2C2CONbits.ACKEN) { ; }    // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) {          // send a STOP:
  I2C2CONbits.PEN = 1;                // communication is complete and master relinquishes bus
  while(I2C2CONbits.PEN) { ; }        // wait for STOP to complete
}

void initExpander(char IO_dir)
{
    ANSELBbits.ANSB2 = 0;       // I2C2 analog off;
    ANSELBbits.ANSB3 = 0;       // I2C2 analog off;
    i2c_master_setup();
    i2c_master_write(IODIR,IO_dir);
}
void setExpander(char pin, char level)
{
    char read_byte,write_byte;
    read_byte = i2c_master_read(OLAT);
    if (level==0)
    {
        write_byte = read_byte&~(pin);
    }
    else if (level==1)
    {
        write_byte = read_byte|pin;
    }
    i2c_master_write(OLAT,write_byte);
}
char getExpander()
{
    return i2c_master_read(GPIO);
}