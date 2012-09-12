#include <Servo.h>

Servo door;

void setup()
{ 
  Serial.begin(9600);  // Hardware serial for Monitor 9600bps
}


void loop()
{
	if (Serial.available() > 0) {
		// read the incoming byte:
		char c = Serial.read();

		if (c == 'L')
                {
                   door.attach(9);
                   delay(500);
                   door.write(0);
                   delay(3000);
                   door.detach();
                }
                else if (c == 'U')
                {
                   door.attach(9);
                   delay(500);
                   door.write(120);
                   delay(3000);
                   door.detach();
                }
	}
}
