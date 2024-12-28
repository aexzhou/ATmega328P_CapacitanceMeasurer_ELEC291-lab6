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
//#include <SoftwareSerial.h>
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

#define RF_SET P2_5

unsigned int cnt = 0;
volatile unsigned int reload;

ISR(TIMER1_COMPA_vect)
{
	OCR1A = OCR1A + reload;
	PORTB ^= 0b00001000; // Toggle PB3
}
void LCD_pulse (void)
{
	LCD_E_1;
	_delay_us(40);
	LCD_E_0;
}

void LCD_byte (unsigned char x)
{
	//Send high nible
	if(x&0x80) LCD_D7_1; else LCD_D7_0;
	if(x&0x40) LCD_D6_1; else LCD_D6_0;
	if(x&0x20) LCD_D5_1; else LCD_D5_0;
	if(x&0x10) LCD_D4_1; else LCD_D4_0;
	LCD_pulse();
	_delay_us(40);
	//Send low nible
	if(x&0x08) LCD_D7_1; else LCD_D7_0;
	if(x&0x04) LCD_D6_1; else LCD_D6_0;
	if(x&0x02) LCD_D5_1; else LCD_D5_0;
	if(x&0x01) LCD_D4_1; else LCD_D4_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS_1;
	LCD_byte(x);
	_delay_ms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS_0;
	LCD_byte(x);
	_delay_ms(5);
}

void LCD_4BIT (void)
{
	LCD_E_0; // Resting state of LCD's enable is zero
	//LCD_RW=0; // We are only writing to the LCD in this program
	_delay_ms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	_delay_ms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, unsigned char clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	_delay_ms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
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
	unsigned int pbflag;
    bool buttonState = PINB & 0b00000100;

	reload=OCR1_RELOAD; // Reload value for default output frequency 
	DDRD|=0b11111000;
	DDRB|=0b00000001; // PB0 (pin 14) as output
	TCCR1B |= _BV(CS10);   // set prescaler to Clock/1
	TIMSK1 |= _BV(OCIE1A); // output compare match interrupt for register A
	
	sei(); // enable global interupt
	usart_init (); // configure the usart and baudrate

	LCD_4BIT();
	DDRB  &= 0b11111001; // Configure PB1 and PB2 as input
	PORTB |= 0b00000110; // Activate pull-up in PB1 and PB2
	pbflag = 0;
	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet

	waitms(500); // Wait for putty to start
	printf("Period measurement using the free running counter of timer 1.\n"
	       "Connect signal to PB1 (pin 15).\n");


	while (1)
	{
	
		count=GetPeriod(100);
		if(count>0)
		{			
			T=count/(F_CPU*100.0);
			f=1/T;
			C=1.44/(f*(1690.0+2.0*1690.0));
			CuF=C*1000000.0;
			buttonState = PINB & 0b00000100;
		    // Button debouncing
		    if(buttonState && !pbflag) {
 		       waitms(50);
			   pbflag = 1; // Toggle flag
	  		} else if (!buttonState && pbflag) {
 		       waitms(50);
			   pbflag = 0; // Toggle flag
    		}
			
				if(pbflag){
				printf("f=%fHz (count=%lu) \n", f, count);
				printf("C=%f uF \n", CuF);
				printf("%d \n", buttonState); 
				printf("\033[A");
				printf("\033[A");
				printf("\033[A");
				LCDprint("Capacitance(uF)   ", 1, 1);
				sprintf(buff, "%guF", CuF);
				LCDprint(buff, 2, 1);
				}
				else{	
				printf("f=%fHz (count=%lu) \n", f, count);
				printf("C=%f uF \n", CuF);
				printf("%d \n", buttonState); 
				printf("\033[A");
				printf("\033[A");
				printf("\033[A");			
				if(CuF > 0.08 && CuF < 0.12){
					LCDprint("Estimated C(uF)", 1, 1);					
					LCDprint("0.1uF, code 104", 2, 1);
				} else if(CuF > 0.8 && CuF < 1.2){
					LCDprint("Estimated C(uF)", 1, 1);					
					LCDprint("1uF, code 105", 2, 1);
				} else if(CuF > 0.008 && CuF < 0.012){
					LCDprint("Estimated C(nF)", 1, 1);							
					LCDprint("10nF, code 103", 2, 1);
				} else if(CuF > 0.0008 && CuF < 0.002){
					LCDprint("estimated C(nF)", 1, 1);
					LCDprint("1nF, code 102", 2, 1);
				} else{
					LCDprint("No estimate, C=", 1, 1);
					sprintf(buff, "%guF", CuF);
					LCDprint(buff, 2, 1);
				}

				}

				}

		else
		{
			printf("NO SIGNAL                     \r");
		}
		waitms(200);
	}
}