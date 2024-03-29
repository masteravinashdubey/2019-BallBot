/************************I2C_LIBRARY***************************/
#ifndef I2C_LIB_H
#define I2C_LIB_H

#include <avr/io.h>

/********typedef for data types***********/
typedef unsigned char 		UINT8;
typedef signed char			INT8;
typedef unsigned short int 	UINT16;
typedef signed short int 	INT16;
typedef enum status_check_cond{ START_ERR=0, SLAVEW_ERR,SLAVER_ERR,WRITE_ERR,READ_ERR,REPSTART_ERR,ACK_ERR,NACK_ERR,OK }STAT;

/**********i2c related terms*************/
#define done 		 	(1<<7)			
#define eack 		 	(1<<6)
#define start		 	(1<<5)
#define stop 		 	(1<<4)
#define i2cen			(1<<2)
#define write			 0x00
#define read			 0x01




/********************************
function name	:	i2c_init
functionality	:	initialise i2c or enabling i2cen bit
arguments		:	none
return value	:	void
*********************************/
void i2c_init()
{
	TWSR = 0x00;
	TWCR = 0x00;
	TWCR = i2cen | done | eack;		//enables i2c , ack bit and clears TWINT bit
	TWBR = 0X0A;					//400khz speed for TWI in 14745600 hz clock frequency
}

/********************************
function name	:	i2c_start
functionality	:	starts tranmission or setting start bit high
arguments		:	none
return value	:	void
********************************/
void i2c_start()
{
	TWCR |= start | eack;			//starting transmission ie TWSTA bit is made high
}

/********************************
function name	:	clear_twint
functionality	:	clears the interrupt bit
arguments		:	none	
return value	:	void
********************************/
void clear_twint()
{
	TWCR |= done;					//clearing TWINT bit
}

/********************************
function name	:	wait
functionality	:	waiting till the data is transmitted or till the interrupt bit gets high
arguments		:	none
return value	:	void
*********************************/
void wait()
{
	while(!(TWCR & done));			//waiting for TWINT bit to be high ie to complete transmission
}

/********************************
function name	:	i2c_stop
functionality	:	stops the transmission or sets the stop bit high
arguments		:	none
return value	:	void
********************************/
void i2c_stop()
{
	TWCR |= stop;					//TWSTO bit is set high
}

/********************************
function name	:	i2c_getstatus	
functionality	:	to find the status of the i2c
arguments		:	none
return value	:	UINT8 (returns the status)
********************************/
UINT8 i2c_getstatus()
{
	UINT8 status;
	status = TWSR & 0xF8;			//5 bits of TWSR register gives the status
//	lcd_print(1,10,status,5);
//	buzzer_on();
//	_delay_ms(20);
//	buzzer_off();
//	while(1);
	return status;
}

/********************************
function name	:	i2c_write
functionality	:	writes a byte of data to TWDR register
arguments		:	UINT8 data(the byte which is to be written)
return value	:	void
*********************************/
void i2c_write(UINT8 data)
{
	TWDR = data;					//writing data or add to TWDR register
}

/********************************
function name	:	i2c_get
functionality	:	reads a byte of data from TWDR register
arguments		:	INT8 *data (points to the byte to which the data needs to be copied)
return value	:	void
*********************************/
void i2c_get(INT8 *data)
{
	*data = TWDR;					//copying data from TWDR register
}

/********************************
function name	:	i2c_sendbyte
functionality	:	sends a byte of data to the slave
arguments		:	UINT8 dev_add (the device address of the slave),
					UINT8 int_add (address of the internal register of the slave),
					UINT8 data(byte of data which needs to be sent)
return value	:	enum (value which tells if the transmission was successful or not)
********************************/
STAT i2c_sendbyte(UINT8 dev_add, UINT8 int_add,UINT8 data)
{
	i2c_start();					//starting transmission
	wait();							//then wait for successful transmission
	if(i2c_getstatus() != 0x08)		//check status
		return START_ERR;

	i2c_write(dev_add | write);		//writing slave + write to TWDR
	TWCR &= ~start;
	clear_twint();					//clear TWINT bit starts transmission
	wait();
	if(i2c_getstatus()!=0x18)
		return SLAVEW_ERR;

	i2c_write(int_add);				//writing int_add to TWDR
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x28)
	{	return WRITE_ERR;	}

	i2c_write(data);				//writing one byte of data...to write multi byte | with 0x80
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x28)
		return WRITE_ERR;

	i2c_stop();

	return OK;
}

/********************************
function name	:	i2c_getbyte
functionality	:	reads a byte of data from the slave
arguments		:	UINT8 dev_add (the device address of the slave),
					UINT8 int_add (address of the internal register of the slave),
					INT8 *data(points to the byte to which the data needs to be copied)					
return value	:	enum (value which tells if the transmission was succesfull or not)
********************************/
STAT i2c_getbyte(UINT8 dev_add,UINT8 int_add,UINT8 *data)
{
	i2c_start();					//starting transmission
	wait();							//then wait for succesfull transmission
	if(i2c_getstatus() != 0x08)		//checking status
		return START_ERR;

	i2c_write(dev_add | write);		//writing slave + write to TWDR
	TWCR &= ~start;
	clear_twint();					//clear TWINT bit and starts transmission
	wait();
	if(i2c_getstatus()!=0x18)
		return SLAVEW_ERR;	

	i2c_write(int_add);				//writing int_add to TWDR
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x28)
		return WRITE_ERR;

	i2c_start();					//repeated start
	wait();
	if(i2c_getstatus() != 0x10)
		return REPSTART_ERR;

	i2c_write(dev_add | read);		//writing slave + read to TWDR register
	TWCR &= ~start;
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x40)
		return SLAVER_ERR;

	TWCR &= ~eack;
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x58)
		return READ_ERR;

	i2c_get(data);					//getting data to the char pointer

	i2c_stop();						//stopping the transmission

	return OK;
}

/********************************
function name	:	i2c_read_multi_byte
functionality	:	reads multiple no of bytes from the slave
arguments		:	UINT8 dev_add (the device address of the slave),
					UINT8 int_add (address of the internal register of the slave),
					UINT16 n(no of bytes which needs to be read)
					INT8 *data(points to the byte to which the data needs to be copied)	
return value	:	enum (value which tells if the transmission was succesfull or not)
*********************************/
STAT i2c_read_multi_byte(UINT8 dev_add,UINT8 int_add,UINT16 n,INT8 *data)
{
	UINT16 i;
	i2c_start();					//starting transmission
	wait();							//then wait for succesfull transmission
	if(i2c_getstatus() != 0x08)		//check status
		return START_ERR;

	i2c_write(dev_add | write);		//writing slave + write to TWDR
	TWCR &=~start;					//to avoid transmitting start bit again
	clear_twint();					//clear TWINT bit and starts transmission
	wait();
	if(i2c_getstatus()!=0x18)
		return SLAVEW_ERR;

	i2c_write(int_add | 0x80);		//to read multi-byte logical or(|) the register with 0x80
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x28)
		return WRITE_ERR;

	i2c_start();					//repeated start
	wait();
	if(i2c_getstatus() != 0x10)
		return REPSTART_ERR;

	i2c_write(dev_add | read);		//writing slave + read to TWDR register
	TWCR &= ~start;
	clear_twint();
	wait();
	if(i2c_getstatus()!=0x40)
		return SLAVER_ERR;

	for(i=0;i<n;i++)
	{
		if(i==(n-1))				//if last byte send nack
		{
			TWCR &= ~eack;			//sending NACK since it is the last bit
			clear_twint();
			wait();
			if(i2c_getstatus()!=0x58)
				return NACK_ERR;	
			i2c_get(&data[i]);		
		}
		else
		{
			TWCR |= eack;			//sending ACK
			clear_twint();
			wait();
			if(i2c_getstatus()!=0x50)
				return ACK_ERR;	

			i2c_get(&data[i]);		//getting required data
		}
	}

	i2c_stop();						//stopping the transmission

	return OK;	
}

#endif 