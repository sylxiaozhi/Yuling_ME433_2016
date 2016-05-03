#ifndef I2C_MASTER_H
#define I2C_MASTER_H
#define SLAVE_ADDR (char) 0x20            // (S) -> 0100 000(R/W) -> byte
char i2c_master_read(char reg_addr);
void i2c_master_write(char reg_addr, char write);
void i2c_master_setup(void);              // set up I2C2 as a master, at 100 kHz
void i2c_master_start(void);              // send a START signal
void i2c_master_restart(void);            // send a RESTART signal
void i2c_master_send(unsigned char byte); // send a byte (either an address or data)
unsigned char i2c_master_recv(void);      // receive a byte of data
void i2c_master_ack(int val);             // send an ACK (0) or NACK (1)
void i2c_master_stop(void);               // send a stop
#endif