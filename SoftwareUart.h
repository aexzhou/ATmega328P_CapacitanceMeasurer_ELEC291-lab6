#define F_CPU 16000000UL

#define sBAUD 9600
#define sOUTPORT PORTD
#define sINPORT  PIND
// #define sTXD (1<<PB0) // PB0 is used as sTXd, Pin14 of ATmega328p (DIP28)
// #define sRXD (1<<PB1) // PB1 is used as sRXD, Pin15 of ATmega328p (DIP28)
#define sTXD (1<<PD3) 
#define sRXD (1<<PD2) 


void ConfigureSoftwareUART (void);
void SendByte (unsigned char c);
unsigned char GetByte (void);
void SendString(char * s);
void GetString(char * s, int nmax);



/* Pinout for DIP28 ATMega328P:

                           -------
     (PCINT14/RESET) PC6 -|1    28|- PC5 (ADC5/SCL/PCINT13)
       (PCINT16/RXD) PD0 -|2    27|- PC4 (ADC4/SDA/PCINT12)
       (PCINT17/TXD) PD1 -|3    26|- PC3 (ADC3/PCINT11)
      (PCINT18/INT0) PD2 -|4    25|- PC2 (ADC2/PCINT10)
 (PCINT19/OC2B/INT1) PD3 -|5    24|- PC1 (ADC1/PCINT9) TX
    (PCINT20/XCK/T0) PD4 -|6    23|- PC0 (ADC0/PCINT8) RX
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