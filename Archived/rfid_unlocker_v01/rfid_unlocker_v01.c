#include <avr/io.h>
#define F_CPU 1000000UL
#include <util/delay.h>
#include <string.h>

#define LOW 0
#define HIGH 1
#define INPUT(port,pin) DDR ## port &= ~(1<<pin)
#define OUTPUT(port,pin) DDR ## port |= (1<<pin)
#define CLEAR(port,pin) PORT ## port &= ~(1<<pin)
#define SET(port,pin) PORT ## port |= (1<<pin)
#define TOGGLE(port,pin) PORT ## port ^= (1<<pin)
#define READ(port,pin) (PIN ## port & (1<<pin))

/* Prototypes */
void ReceiveByte (void);
void lockServo (void);
void unlockServo (void);
void killServo (void);
void readRFID (void);
void doorSeq (void);

/* Variables */
char code[10];
int bytesread = 0;
int doorUnlocked = 0;
char validKey[] = "2200536C73";   //Eric's Card
char validKey2[] = "2100DFC897";  //Eric's Dime
char validKey3[] = "2200776E9C";  //Chass' Card
char validKey4[] = "36005B7320";  //Chass' Keychain
char receivedByte;

/* Main Function */
int main (void)
{
	/* Setup the serial port */
	UBRRL = 25; // Set the baudrate to 2400 bps using the internal 8MHz crystal / prescaler of 8
	UCSRB = (1 << RXEN); // Enable UART receiver
	UCSRC = (1 << UCSZ1)|(1 << UCSZ0); // 8 data bits, 1 stop bit

	/* Setup the servo */
	DDRB |= (1<<PB3); // OC1A output
	ICR1 = 20000; // TOP, set for 50Hz (20ms) - As to spec for HS-311 Servo Refresh Rate
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS10); // Timer 1 fast PWM mode 14, Clear on compare, Set at TOP, No Prescaler

	OUTPUT(D, 3); // port D, pin 3 as output 
	CLEAR(D, 3); // set port D pin 3 to LOW
	INPUT(D, 4); // port D, pin 4 as input 
	SET(D, 4); // set port D pin 4 to HIGH
  
	while (1)
	{
		switch(doorUnlocked) 
		{
    			case 0: // Wait for an RFID tag
				readRFID();       	
				break;
			case 1: // RFID tag valid, open the door
				doorSeq();
				break;
		}
	}
}

/* Setup the Servo/PWM Stuff */
void lockServo (void)
{
	// Set signal - PWM * 7.843 + 500
	OCR1A = 650;

	// Enable PWM
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}
void unlockServo (void)
{
	// Set signal - PWM * 7.843 + 500
	OCR1A = 1750;
	
	// Enable PWM
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}
void killServo (void)
{
	// Disable PWM
	TCCR1A = (0<<COM1A1)|(0<<WGM11);
}

void doorSeq(void)
{
	unlockServo();
	_delay_ms(1000);
	killServo();
	_delay_ms(3000);
	while (1) {
		// wait for door close (pin goes low for at least 100ms)
		if (READ(D, 4) == LOW) {
			_delay_ms(100);
			if (READ(D, 4) == LOW)
				break;
		}
	}
	_delay_ms(500);
	lockServo();
	_delay_ms(1000);
	killServo();
	doorUnlocked = 0;
}

/* Serial Read Function */
void ReceiveByte (void)
{
  /* Wait for incoming data */
  while (!(UCSRA & (1 << RXC)));
	
  /* Return the data */
  receivedByte = UDR;
}

void readRFID (void)
{
	ReceiveByte();
	if(receivedByte == 10) // check for header
	{
		bytesread = 0;
		while(bytesread < 10) // read 10 digit code
		{
			ReceiveByte();
			if((receivedByte == 10) || (receivedByte == 13)) // detect header/stop while reading and auto-reset
				break; // stop reading
			code[bytesread] = receivedByte; // add the digit      
			bytesread++; // ready to read next digit
		}
		if(bytesread == 10) // if 10 digit read is complete
		{
			// Valid Tag Check
			if (strncmp(code, validKey, 10) == 0 || strncmp(code, validKey2, 10) == 0 || strncmp(code, validKey3, 10) == 0 || strncmp(code, validKey4, 10) == 0)
				doorUnlocked = 1;
		}
	}
}
