#include <avr/io.h>
#define F_CPU 1000000UL
#include <util/delay.h>
#include <string.h>

void doorHandler (void);
void receiveByte (void);
void unlockServo (void);
void lockServo (void);
void killServo (void);
void disableSerial (void);
void enableSerial (void);

/* I/O Macros */
#define LOW 0
#define HIGH 1
#define INPUT(port,pin) DDR ## port &= ~(1<<pin)
#define OUTPUT(port,pin) DDR ## port |= (1<<pin)
#define CLEAR(port,pin) PORT ## port &= ~(1<<pin)
#define SET(port,pin) PORT ## port |= (1<<pin)
#define TOGGLE(port,pin) PORT ## port ^= (1<<pin)
#define READ(port,pin) (PIN ## port & (1<<pin))

/* Globals */
char readBytes[10];				//A place to store tag bytes
unsigned int bytesRead = 0;			//Number of bytes read thus far
char validTag = LOW;				//Tracks if we've received a valid tag
char doorOpen = HIGH; 				//Tracks if the door has been opened since last loop through
char validKey[] = "TAG_VALUE_GOES_HERE";  	//Valid RFID Tag


/* Main Function */
int main (void)
{
	/* Setup the serial port */
	UBRRL = 25; // Set the baudrate to 2400 bps using the internal 8MHz crystal / prescaler of 8
  	UCSRC = (1 << UCSZ1) | (1 << UCSZ0);	// 8 Data bits, 1 stop bit
	enableSerial();	// Enable reading from the serial port
	
	/* Setup the servo */
	DDRB |= (1<<PB3); // OC1A output
	ICR1 = 20000; // TOP, set for 50Hz (20ms) - As to spec for HS-311 Servo Refresh Rate
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS10); // Timer 1 fast PWM mode 14, Clear on compare, Set at TOP, No Prescaler
	
	INPUT(D, 5); // port D, pin 5 as input 		//DOOR AJAR (Configure as Input)
	SET(D, 5); // set port D pin 5 to HIGH		//DOOR AJAR (Enable Pull-up Resistor)
	INPUT(D, 4); // port D, pin 4 as input		//DOOR RELEASE	(Configure as Input)
	SET(D, 4); // set port D pin 4 to HIGH		//DOOR RELEASE	(Enable Pull-up Resistor)
  
	while (1)
	{
		doorHandler();
		receiveByte();
	}
}

void doorHandler (void) {
	if (READ(D, 4) == LOW || validTag == HIGH) { 	//Door release has been pressed or a valid tag was presented
		disableSerial();
		unlockServo();
		_delay_ms(1000);	//Delay to unlock door
		killServo();
		_delay_ms(3000);	//Delay to wait for door to open
		doorOpen = HIGH;
		validTag = LOW;
		enableSerial();
	}
	else if (doorOpen && READ(D, 5) == LOW) {	//The door was open and is now closed
		disableSerial();
		_delay_ms(100);
		if (READ(D, 5) == LOW) {
			_delay_ms(1000);	//Delay to lock door
			lockServo();
			_delay_ms(1000);
			killServo();
			doorOpen = LOW;
		}
		enableSerial();
	}
	else if (!doorOpen && READ(D, 5) != LOW) {	//Detect if door was opened
		_delay_ms(100);
		if (READ(D, 5) != LOW)
			doorOpen = HIGH;
	}
}

void receiveByte (void)
{
	// Check for new data
	if (UCSRA & (1 << RXC)) {
		char receivedByte = UDR;
	
		if (receivedByte == 0x0A) {		// This is the start byte - reset byte count
			bytesRead = 0;
		}
		else if (receivedByte == 0x0D) {	// This is the stop byte - we now have a tag
			if (strncmp(readBytes, validKey, 10) == 0)
				validTag = HIGH;
		}
		else {
			readBytes[bytesRead++] = receivedByte;	// Store each byte
		}
	}
}

void unlockServo (void)
{
	// Set signal - PWM Value(0-255) * 7.843 + 500
	OCR1A = 1100;	// This sets the unlock position of the servo

	// Enable PWM
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}

void lockServo (void)
{
	// Set signal - PWM Value(0-255) * 7.843 + 500
	OCR1A = 2200;	// This sets the lock position of the servo
	
	// Enable PWM Output
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}

void killServo (void)
{
	// Disable PWM Output
	TCCR1A = (0<<COM1A1)|(0<<WGM11);
}

void disableSerial (void)
{
	UCSRB = 0;
}

void enableSerial (void)
{
	UCSRB = (1 << RXEN);
}