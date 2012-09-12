#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>

/* I/O Macros */
#define LOW 0
#define HIGH 1
#define INPUT(port,pin) DDR ## port &= ~(1<<pin)
#define OUTPUT(port,pin) DDR ## port |= (1<<pin)
#define CLEAR(port,pin) PORT ## port &= ~(1<<pin)
#define SET(port,pin) PORT ## port |= (1<<pin)
#define TOGGLE(port,pin) PORT ## port ^= (1<<pin)
#define READ(port,pin) (PIN ## port & (1<<pin))

/* Variables */
volatile unsigned int bitsread = 0;
volatile unsigned char recKey[3];
unsigned char siteKey = 28;
unsigned char aptSiteKey = 10;
unsigned int ebKey = 11386;
unsigned int ebAptKey = 1303;
unsigned int esAptKey = 1332;
unsigned int esKey = 11397;
unsigned int cbKey = 11262;
unsigned int afKey = 13336;
unsigned int bgKey = 14451;
char doorOpen = 1; //Tracks if the door has been opened since last loop through

/* Main Function */
int main (void)
{
	/* Setup the serial port */
	UBRRL = 25; // Set the baudrate to 2400 bps using the internal 8MHz crystal / prescaler of 8
	UCSRB = (1 << RXEN) | (1 << TXEN);	// Enable UART
  	UCSRC = (1 << UCSZ1) | (1 << UCSZ0);	// 8 Data bits, 1 stop bit
	
	/* Setup the servo */
	DDRB |= (1<<PB3); // OC1A output
	ICR1 = 20000; // TOP, set for 50Hz (20ms) - As to spec for HS-311 Servo Refresh Rate
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS10); // Timer 1 fast PWM mode 14, Clear on compare, Set at TOP, No Prescaler
	
	INPUT(D, 4); // port D, pin 4 as input 
	SET(D, 4); // set port D pin 4 to HIGH
	INPUT(D, 3); // port D, pin 5 as input 
	SET(D, 3); // set port D pin 5 to HIGH
  
	while (1)
	{
		checkDoor();
		ReceiveByte();
		processData();
		_delay_ms(20);	//We need to wait to make sure we receive more serial data
	}
}

void checkDoor (void) {
	if (READ(D, 3) == LOW)	//Door release has been pressed
		doorSeq();
	else if (doorOpen && READ(D, 4) == LOW) {	//The door has been forced open and has just been closed again
		_delay_ms(100);
		if (READ(D, 4) == LOW) {
			_delay_ms(1000);
			lockServo();
			_delay_ms(1000);
			killServo();
		}
		doorOpen = 0;
	}
	else if (READ(D, 4) != LOW) {	//Detect if someone forced the door open manually
		_delay_ms(100);
		if (READ(D, 4) != LOW)
			doorOpen = 1;
	}
}

void ReceiveByte (void)
{
	if (UCSRA & (1 << RXC))
		recKey[bitsread++] = UDR;
	else
		bitsread = 0;
}

void processData (void)
{
	if(bitsread >= 3) // 24 bit code received - check the ID read
	{
		unsigned int cardKey = (unsigned int)recKey[1] << 8;
		cardKey |= (unsigned int)recKey[2];
		if (recKey[0] == siteKey) {
			if (cardKey == ebKey || cardKey == esKey || cardKey == cbKey || cardKey == afKey || cardKey == bgKey)
				doorSeq();
		}
		else if (recKey[0] == aptSiteKey) {
			if (cardKey == ebAptKey)
				doorSeq();
			else if (cardKey == esAptKey)
				doorSeq();
		}
		bitsread = 0;
	}
}

/* Door sequencing */
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
	_delay_ms(1000);
	lockServo();
	_delay_ms(1000);
	killServo();
}

void unlockServo (void)
{
	// Set signal - PWM * 7.843 + 500
	OCR1A = 1100;

	// Enable PWM
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}

void lockServo (void)
{
	// Set signal - PWM * 7.843 + 500
	OCR1A = 2200;
	
	// Enable PWM
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}

void killServo (void)
{
	// Disable PWM
	TCCR1A = (0<<COM1A1)|(0<<WGM11);
}
