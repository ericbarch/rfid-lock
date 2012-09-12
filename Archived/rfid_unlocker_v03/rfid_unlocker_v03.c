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
void processData (void);
volatile int bitsread = 0;
volatile int last_bitsread = 0;
volatile int recKey[26];
unsigned int siteKey = 28;
//unsigned int siteKey = 10; - Apartments
unsigned int ebKey = 11386;
unsigned int cbKey = 11262;
unsigned int afKey = 11386;
unsigned int esKey = 11386;
unsigned int bgKey = 11386;
int doorOpen = 1; //Tracks if the door has been opened since last loop through

/* Main Function */
int main (void)
{
	/* Setup the servo */
	DDRB |= (1<<PB3); // OC1A output
	ICR1 = 20000; // TOP, set for 50Hz (20ms) - As to spec for HS-311 Servo Refresh Rate
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS10); // Timer 1 fast PWM mode 14, Clear on compare, Set at TOP, No Prescaler

	/* Interrupts */
	MCUCR = (1<<ISC01) | (1<<ISC11);
	GIMSK = (1<<INT0) | (1<<INT1);
	sei(); // Enable all interrupts

	INPUT(D, 4); // port D, pin 4 as input
	SET(D, 4); // set port D pin 4 to HIGH
  
	while (1)
	{
		processData();
	}
}

/* Setup the Servo/PWM Functions */
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
	OCR1A = 1950;
	
	// Enable PWM
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
}

void killServo (void)
{
	// Disable PWM
	TCCR1A = (0<<COM1A1)|(0<<WGM11);
}

void openDoor(void)
{
	unlockServo();
	_delay_ms(1000);
	killServo();
	_delay_ms(3000);
	doorOpen = 1;
}

// Convert a binary array to a decimal number
int bin2dec(int type)
{
	int b, k, m, kmin, kmax;
	int sum = 0;
	if (type == 0) { //Site code
		kmin = 1;
		kmax = 8;
	}
	else { //Card code
		kmin = 9;
		kmax = 24;
	}
	for(k = kmin; k <= kmax; k++)
	{
		for(b = 1, m = kmax; m > k; m--)
		{
			b *= 2;
		}
		sum = sum + recKey[k] * b;
	}
	return(sum);
}

void processData (void)
{
	if(bitsread >= 26) // 26 bit code received - check the ID read
	{
		if (bin2dec(0) == siteKey) {
			if (bin2dec(1) == ebKey)
				openDoor();
			else if (bin2dec(1) == cbKey)
				openDoor();
			else if (bin2dec(1) == afKey)
				openDoor();
			else if (bin2dec(1) == esKey)
				openDoor();
			else if (bin2dec(1) == bgKey)
				openDoor();
		}
		bitsread = 0;
	}

	if (doorOpen && READ(D, 4) == LOW) {	//The door has been unlocked and has just been closed again
		_delay_ms(100);
		if (READ(D, 4) == LOW) {
			_delay_ms(1250);
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

	//Clear ghost bits
	_delay_ms(100);
	if (bitsread == last_bitsread)
		bitsread = 0;
	last_bitsread = bitsread;
}

SIGNAL (SIG_INT0)
{
	recKey[bitsread++] = 0;
}

SIGNAL (SIG_INT1)
{
	recKey[bitsread++] = 1;
}
