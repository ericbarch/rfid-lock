#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>

/* Variables */
int bitsread = 0;
int last_bitsread = 0;
long int incomingKey = 0;

/* Main Function */
int main (void)
{
	/* Interrupts */
	MCUCR = (1<<ISC01) | (1<<ISC11);
	GIMSK = (1<<INT0) | (1<<INT1);
	sei(); // Enable all interrupts

	/* Setup the serial port */
	UBRRL = 25; // Set the baudrate to 2400 bps using the internal 8MHz crystal / prescaler of 8
  	UCSRB = (1 << RXEN) | (1 << TXEN);	// Enable UART
  	UCSRC = (1 << UCSZ1) | (1 << UCSZ0);	// 8 Data bits, 1 stop bit

	while (1)
	{
		processData();
	}
}

void processData (void)
{
	if(bitsread >= 26) // 26 bit code received
	{
		unsigned char siteKey = incomingKey >> 17;
		unsigned char upperKey = incomingKey >> 9;
		unsigned char lowerKey = incomingKey >> 1;
		TransmitByte(siteKey);
		TransmitByte(upperKey);
		TransmitByte(lowerKey);	
		bitsread = 0;
		incomingKey = 0;
	}

	//Clear ghost bits
	_delay_ms(100);
	if (bitsread == last_bitsread) {
		bitsread = 0;
		incomingKey = 0;
	}
	last_bitsread = bitsread;
}

void TransmitByte (unsigned char data)
{
	/* Wait for empty transmit buffer */
  	while (!(UCSRA & (1 << UDRE)));

  	/* Transmit byte */
  	UDR = data;
}

SIGNAL (SIG_INT0)
{
	incomingKey = incomingKey << 1;
	bitsread++;
}

SIGNAL (SIG_INT1)
{
	incomingKey = (incomingKey << 1) + 1;
	bitsread++;
}
