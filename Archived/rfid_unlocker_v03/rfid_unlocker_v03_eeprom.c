#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/eeprom.h>

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
volatile int recKey[26];
int ramKey[2];

int EEMEM rfid_data[2]= {1, 1};

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

void openDoor(void)
{
	unlockServo();
	_delay_ms(1000);
	killServo();
	_delay_ms(3000);
	_delay_ms(1250);
	lockServo();
	_delay_ms(1000);
	killServo();
	bitsread = 0;
}

void processData (void)
{
	if(bitsread >= 26) // 26 bit code received
	{
		eeprom_read_block(&ramKey[0], &rfid_data[0], 2);
		_delay_ms(500);
		if (ramKey[1] == 0) {
			openDoor();
		}
		bitsread = 0;
	}
}

SIGNAL (SIG_INT0)
{
	recKey[bitsread++] = 0;
}

SIGNAL (SIG_INT1)
{
	recKey[bitsread++] = 1;
}
