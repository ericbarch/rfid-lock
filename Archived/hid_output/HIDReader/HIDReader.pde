void setup()
{
  Serial.begin(9600);	// opens serial port, sets data rate to 9600 bps
  attachInterrupt(0, writezero, FALLING);
  attachInterrupt(1, writeone, FALLING);
}

void loop()
{
  //do nothing
}

void writezero()
{
   Serial.print("0");
}

void writeone()
{
  Serial.print("1");
}
