#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>

void doorHandler (void);
void processData (void);
void unlockServo (void);
void lockServo (void);
void killServo (void);

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
volatile long int incomingKey = 0;		//Incoming tag bits are stored here
volatile unsigned int bitsread = 0;		//Number of bits read
unsigned int last_bitsread = 0;			//Last loop's number of bits
char validTag = LOW;				//Tracks if we've received a valid tag
char doorOpen = HIGH; 				//Tracks if the door has been opened since last loop through
unsigned char siteKey = 11;			//Site code of the target tag
unsigned int uniqueKey = 11111;			//Unique code of the target tag


/* Main Function */
int main (void)
{
	/* Interrupts */
	MCUCR = (1<<ISC01) | (1<<ISC11);
	GIMSK = (1<<INT0) | (1<<INT1);
	sei(); // Enable all interrupts
	
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
		processData();
	}
}

void doorHandler (void) {
	if (READ(D, 4) == LOW || validTag == HIGH) { 	//Door release has been pressed or a valid tag was presented
		unlockServo();
		_delay_ms(1000);	//Delay to unlock door
		killServo();
		_delay_ms(3000);	//Delay to wait for door to open
		doorOpen = HIGH;
		validTag = LOW;
	}
	else if (doorOpen && READ(D, 5) == LOW) {	//The door was open and is now closed
		_delay_ms(100);
		if (READ(D, 5) == LOW) {
			_delay_ms(1000);	//Delay to lock door
			lockServo();
			_delay_ms(1000);
			killServo();
			doorOpen = LOW;
		}
	}
	else if (!doorOpen && READ(D, 5) != LOW) {	//Detect if door was opened
		_delay_ms(100);
		if (READ(D, 5) != LOW)
			doorOpen = HIGH;
	}
}

void processData (void)
{
	if(bitsread == 26) // 26 bit code received
	{
		unsigned char recSite = incomingKey >> 17;
		unsigned int recKey = incomingKey >> 1;
		
		// Check for a valid key
		if (recSite == siteKey && recKey == uniqueKey)
			validTag = HIGH;
		
		bitsread = 0;
		incomingKey = 0;
	}

	//Clear ghost bits
	_delay_ms(50);	// We receive bits through wiegand every 2mS, this should be plenty
	if (bitsread < 26 && bitsread == last_bitsread) {
		bitsread = 0;
		incomingKey = 0;
	}
	last_bitsread = bitsread;
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

SIGNAL (SIG_INT0)
{
	incomingKey = incomingKey << 1;
	bitsread++;
}

SIGNAL (SIG_INT1)
{
	incomingKey = incomingKey << 1;
	incomingKey |= 1;
	bitsread++;
}