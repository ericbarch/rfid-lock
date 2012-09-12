#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>

#define LOW 0
#define HIGH 1
#define INPUT(port,pin) DDR ## port &= ~(1<<pin)
#define OUTPUT(port,pin) DDR ## port |= (1<<pin)
#define CLEAR(port,pin) PORT ## port &= ~(1<<pin)
#define SET(port,pin) PORT ## port |= (1<<pin)
#define TOGGLE(port,pin) PORT ## port ^= (1<<pin)
#define READ(port,pin) (PIN ## port & (1<<pin))

/* Prototypes */
void lockServo (void);
void unlockServo (void);
void killServo (void);
void processBitStream (void);
void doorSeq (void);

/* Variables */
int bytesread = 0;
int invalidKey = 0;
int doorUnlocked = 0;
int validKeyE[26]= {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0};
int validKeyC[26]= {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1};
int IDguess = 0; //1 - Eric, 0 - Chass

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
		switch(doorUnlocked) 
		{
    			case 0: // Wait for an RFID tag
				processBitStream();       	
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
	OCR1A = 1950;
	
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
	_delay_ms(1250);
	lockServo();
	_delay_ms(1000);
	killServo();
	bytesread = 0;
	doorUnlocked = 0;
}

void processBitStream (void)
{
	if(bytesread == 26) // 26 bit code received
	{
		if (invalidKey == 0)
			doorUnlocked = 1;
		else {
			invalidKey = 0;
			bytesread = 0;
		}
	}
}

SIGNAL (SIG_INT0)
{
	if (bytesread < 14 && validKeyE[bytesread] != 0)
		invalidKey = 1;
	else if (bytesread == 14)
		IDguess = 0;
	else if (IDguess == 0 && bytesread < 26) {
		if (bytesread < 26 && validKeyC[bytesread] != 0)
		invalidKey = 1;
	}
	else if (IDguess == 1 && bytesread < 26) {
		if (bytesread < 26 && validKeyE[bytesread] != 0)
		invalidKey = 1;
	}
	bytesread++;
}

SIGNAL (SIG_INT1)
{
	if (bytesread < 14 && validKeyE[bytesread] != 1)
		invalidKey = 1;
	else if (bytesread == 14)
		IDguess = 1;
	else if (IDguess == 0 && bytesread < 26) {
		if (bytesread < 26 && validKeyC[bytesread] != 1)
		invalidKey = 1;
	}
	else if (IDguess == 1 && bytesread < 26) {
		if (bytesread < 26 && validKeyE[bytesread] != 1)
		invalidKey = 1;
	}
	bytesread++;
}
