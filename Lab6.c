// This program shows how to measure the period of a signal using timer 1 free running counter.
#define DEF_FREQ 15000L
#define OCR1_RELOAD ((F_CPU/(2*DEF_FREQ))+1)
#define F_CPU 16000000UL
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usart.h"
#include "lcd.h"




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

int main(void)
{
	long int count;
	float T, f, C, CuF;
	unsigned int adc;
	char buff[32];
	unsigned long newF;
	
	reload=OCR1_RELOAD; // Reload value for default output frequency 

	DDRB=0b00000011; // PB1 (pin 15) and PB0 (pin 14) are our outputs
	PORTB |= 0x01; // PB0=NOT(PB1)
	TCCR1B |= _BV(CS10);   // set prescaler to Clock/1
	TIMSK1 |= _BV(OCIE1A); // output compare match interrupt for register A
	
	sei(); // enable global interupt
	usart_init (); // configure the usart and baudrate
	adc_init();
	
	DDRB  &= 0b11111101; // Configure PB1 as input
	PORTB |= 0b00000010; // Activate pull-up in PB1

	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet

	waitms(500); // Wait for putty to start
	printf("Period measurement using the free running counter of timer 1.\n"
	       "Connect signal to PB1 (pin 15).\n");
	
	while (1)
	{
		printf("Frequency: ");
    	usart_gets(buff, sizeof(buff)-1);
    	newF=atol(buff);

	    if(newF>111000L)
	    {
	       printf("\r\nWarning: The maximum frequency that can be generated is around 111000Hz.\r\n");
	       newF=111000L;
	    }
	    if(newF>0)
	    {
			reload=(F_CPU/(2L*newF))+1;  
		    printf("\r\nFrequency set to: %ld\r\n", F_CPU/((reload-1)*2L));
        }
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
			

			if(flag == 0){
				LCDprint("Capacitance(uF)   ", 1, 1);
				sprintf(buffer, "%guF", C);
				LCDprint(buffer, 2, 1);
			} else if(flag == 1){
				LCDprint("Frequency(Hz)   ", 1, 1);
				sprintf(buffer, "%luHz", f);
				LCDprint(buffer, 2, 1);		
			} else if(flag == 2){
				LCDprint("Period(s)   ", 1, 1);
				sprintf(buffer, "%fS", T);
				LCDprint(buffer, 2, 1);		
			}

		}
		else
		{
			printf("NO SIGNAL                     \r");
		}
		waitms(200);
	}
}