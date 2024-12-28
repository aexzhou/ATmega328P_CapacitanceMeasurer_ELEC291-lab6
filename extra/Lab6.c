// This program shows how to measure the period of a signal using timer 1 free running counter.
#define DEF_FREQ 15000L
#define OCR1_RELOAD ((F_CPU/(2*DEF_FREQ))+1)
#define F_CPU 16000000UL
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usart.h"
#include "lcd.h"
#include "SoftwareUart.h"

// #define RED (1<<PC5)
// #define YELLOW (1<<PC4)
// #define RF_SET (1<<PC2)
#define RED "PC3"
#define YELLOW "PC4"
#define BUFSIZE 80



/* Pinout for DIP28 ATMega328P:

                           -------
     (PCINT14/RESET) PC6 -|1    28|- PC5 (ADC5/SCL/PCINT13)
       (PCINT16/RXD) PD0 -|2    27|- PC4 (ADC4/SDA/PCINT12)
       (PCINT17/TXD) PD1 -|3    26|- PC3 (ADC3/PCINT11)
      (PCINT18/INT0) PD2 -|4    25|- PC2 (ADC2/PCINT10)
 (PCINT19/OC2B/INT1) PD3 -|5    24|- PC1 (ADC1/PCINT9)
    (PCINT20/XCK/T0) PD4 -|6    23|- PC0 (ADC0/PCINT8)
                     VCC -|7    22|- GND
                     GND -|8    21|- AREF
(PCINT6/XTAL1/TOSC1) PB6 -|9    20|- AVCC
(PCINT7/XTAL2/TOSC2) PB7 -|10   19|- PB5 (SCK/PCINT5)
   (PCINT21/OC0B/T1) PD5 -|11   18|- PB4 (MISO/PCINT4)
 (PCINT22/OC0A/AIN0) PD6 -|12   17|- PB3 (MOSI/OC2A/PCINT3)
      (PCINT23/AIN1) PD7 -|13   16|- PB2 (SS/OC1B/PCINT2)
  (PCINT0/CLKO/ICP1) PB0 -|14   15|- PB1 (OC1A/PCINT1)
                           -------
*/


unsigned int cnt = 0;
volatile unsigned int reload;

ISR(TIMER1_COMPA_vect)
{
	OCR1A = OCR1A + reload;
	PORTB ^= 0b00000011; // Toggle PB0 and PB1
	//PORTD ^= 0b11000000;
	//		   76543210
}

void wait_1ms(void)
{
	unsigned int saved_TCNT1;
	
	saved_TCNT1=TCNT1;
	
	while((TCNT1-saved_TCNT1)<(F_CPU/1000L)); // Wait for 1 ms to pass
}

void waitms(int ms)
{
	while(ms--) wait_1ms();
}

#define PIN_PERIOD (PINB & 0b00000010)

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
long int GetPeriod (int n)
{
	int i, overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	overflow=0;
	TIFR1=1; // TOV1 can be cleared by writing a logic one to its bit location.  Check ATmega328P datasheet page 113.
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	overflow=0;
	TIFR1=1;
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	
	overflow=0;
	TIFR1=1;
	saved_TCNT1a=TCNT1;
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
	}
	saved_TCNT1b=TCNT1;
	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
}

// Toglle pins for me so i dont have to do it the long shitty way
void setPin(char* pinName, int val) {
    if(val != 0 && val != 1) { // check input validity
        return;
    }
    // get port and pin from pin name
    if(pinName[0] == 'P') {
        char port = pinName[1];
        int pin = pinName[2] - '0'; // covert ascii to int
        // set ddr (data direction register)
        switch (port) {
            case 'B':
                DDRB |= (1 << pin);
                if(val == 1)
                    PORTB |= (1 << pin); // set portb high (1)
                else
                    PORTB &= ~(1 << pin); // set portb low (0)
                break;
            case 'C':
                DDRC |= (1 << pin);
                if(val == 1)
                    PORTC |= (1 << pin);
                else
                    PORTC &= ~(1 << pin);
                break;
            case 'D':
                DDRD |= (1 << pin);
                if(val == 1)
                    PORTD |= (1 << pin);
                else
                    PORTD &= ~(1 << pin);
                break;
            default:
                break; 
        }
    }
}

void SendATCommand (char * s)
{	
	char buffer[BUFSIZE];
	printf("Command: %s", s);
	PORTC &= ~(1<<2); // 'set' pin to 0 is 'AT' mode.
	
	waitms(5);
	printf("1");
	SendString(s);
	printf("2");
	GetString(buffer, BUFSIZE-1);
	printf("3");
	waitms(10);
	printf("4");
	PORTC |= (1<<2); // 'set' pin to 1 is normal operation mode.
	printf("5");
	printf("Response: %s\r\n", buffer);
	printf("6");
}

void Init_RF(void){
	// We should select an unique device ID.  The device ID can be a hex
	// number from 0x0000 to 0xFFFF.  In this case is set to 0xABBA
	SendATCommand("AT+DVID1357\r\n");  
	// To check configuration
	SendATCommand("AT+VER\r\n");
	SendATCommand("AT+BAUD\r\n");
	SendATCommand("AT+RFID\r\n");
	SendATCommand("AT+DVID\r\n");
	SendATCommand("AT+RFC\r\n");
	SendATCommand("AT+POWE\r\n");
	SendATCommand("AT+CLSS\r\n");
}


int main(void)
{
	long int count;
	float T, f, C, CuF;
	unsigned int adc;
	char buff[BUFSIZE];
	unsigned long newF;
	//char flag = 0;
	reload=OCR1_RELOAD; // Reload value for default output frequency 
	//bit: 76543210 // 1 = output for DDRn
	DDRB=0b00000011; // PB1 (pin 15) and PB0 (pin 14) are our outputs
	DDRC=0b00011000; // Creg outputs for LEDS
	DDRD |= 0b00010000; // set DDRD[4] to 1 output for "SET" for JDY-40
	PORTB |= 0x01; // PB0=NOT(PB1)
	TCCR1B |= _BV(CS10);   // set prescaler to Clock/1
	TIMSK1 |= _BV(OCIE1A); // output compare match interrupt for register A
	usart_init();
	sei(); // enable global interupt
	
	//bit:     76543210
	DDRB  &= 0b11111101; // Configure PB1 as input
	PORTB |= 0b00000010; // Activate pull-up in PB1
	
	ConfigureSoftwareUART(); // configure software UART for RF module

	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet

	waitms(500); // Wait for putty to start
	printf("Period measurement using the free running counter of timer 1.\n"
	       "Connect signal to PB1 (pin 15).\n");
	Init_RF();
	
	while (1)
	{
		// printf("Frequency: ");
    	// usart_gets(buff, sizeof(buff)-1);
    	// newF=atol(buff);

	    // if(newF>111000L)
	    // {
	    //    printf("\r\nWarning: The maximum frequency that can be generated is around 111000Hz.\r\n");
	    //    newF=111000L;
	    // }
	    // if(newF>0)
	    // {
		// 	reload=(F_CPU/(2L*newF))+1;  
		//     printf("\r\nFrequency set to: %ld\r\n", F_CPU/((reload-1)*2L));
        // }
		count=GetPeriod(100);
		if(count>0)
		{
			T=count/(F_CPU*100.0);
			f=1/T;
			C=1.44/(f*(1690.0+2.0*1690.0));
			CuF=C*1000000.0;
			printf("f=%fHz (count=%lu) \n", f, count);
			printf("C=%f uF \n", CuF);

			printf("\033[A");
			printf("\033[A");

			// if(flag == 0){
			// 	LCDprint("Capacitance(uF)   ", 1, 1);
			// 	sprintf(buff, "%guF", C);
			// 	LCDprint(buff, 2, 1);
			// } else if(flag == 1){
			// 	LCDprint("Frequency(Hz)   ", 1, 1);
			// 	sprintf(buff, "%luHz", f);
			// 	LCDprint(buff, 2, 1);		
			// } else if(flag == 2){
			// 	LCDprint("Period(s)   ", 1, 1);
			// 	sprintf(buff, "%fS", T);
			// 	LCDprint(buff, 2, 1);		
			// }

		}
		else
		{
			printf("NO SIGNAL                     \r");
		}

		//sprintf(buff,"f=%fHz", f);
		strcpy(buff, "test");
		SendString(buff);
		//setPin(RED,0); // red on
		//setPin(YELLOW,1); //yellow off
		//PORTC |= 0x03;
		//PORTC &= ~(1<<0x03);
		//PORTC &= ~(1<<0x04);
		//PORTC |= (1<<0x04);
		
		
		waitms(200);
	}
}