#include "SoftwareUart.h"
#include <avr/io.h>
#include <util/delay.h>

unsigned char echo=0;

void ConfigureSoftwareUART (void)
{
	DDRD |= sTXD;    // Configure pin sTXD as output
	PORTD |= sTXD;   // Default state of transmit pin is 1
	DDRD &= (~sRXD); // Configure pin sRXD as input
}

void SendByte (unsigned char c)
{
	unsigned char i;
	
	// Send start bit
	sOUTPORT &= (~sTXD);
  	_delay_us(1E6/sBAUD);
  	// Send 8 data bits
	for (i=0; i<8; i++)
  	{
    	if( c & 1 )
    	{
      		sOUTPORT |= sTXD;
      	}
    	else
      	{
      		sOUTPORT &= (~sTXD);
      	}
    	c >>= 1;
		_delay_us(1E6/sBAUD);
 	}
 	// Send the stop bit
	sOUTPORT |= sTXD;
	_delay_us(1E6/sBAUD);
}

unsigned char GetByte (void)
{
	unsigned char c, i;
	
	// Wait for input pin to change to zero (start bit)
	printf("B");
	while(sINPORT&sRXD); // bug: always true, inf loop, never changes to 0
	// Wait one and a half bit-time to sample in the middle of incomming bits
	printf("C");
	_delay_us((3*1E6)/(2*sBAUD));
	// Receive 8 data bits
	for (i=0, c=0; i<8; i++)
  	{
  		c>>=1;
  		if(sINPORT&sRXD) c|=0x80;
		_delay_us(1E6/sBAUD);
	}
	if(echo==1) SendByte(c); // If echo activated send back what was received
	if(c==0x05) // Control+E activates echo
	{
		echo=1;
	}
	if(c==0x06) // Control+F de-activates echo
	{
		echo=0;
	}
	return c;
}

void SendString(char * s)
{
	while(*s != 0) SendByte(*s++);
}

void GetString(char * s, int nmax)
{
	unsigned char c;
	int n;
	
	while(1)
	{	
		printf("A");
		c=GetByte();
		if( (c=='\n') || (c=='\r') || n==(nmax-1) )
		{
			*s=0;
			return;
		}
		else
		{
			*s=c;
			s++;
			n++;
		}
	}
}

// int main(void)
// {
// 	char buff[80];
// 	unsigned char i;
	
// 	ConfigureSoftwareUART();
// 	SendString("Hello, World!  Using software (bit-bang) UART\r\n");
// 	while(1) 
// 	{
// 		SendString("\r\nType Something: ");
// 		GetString(buff, sizeof(buff)-1);
// 		SendString("\r\nYou typed: ");
// 		SendString(buff);		
// 	}
// 	return 0;
// }
