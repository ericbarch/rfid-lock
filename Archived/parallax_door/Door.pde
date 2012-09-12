#include <SoftwareSerial.h>
#include <Servo.h>

int val = 0; 
char code[10];
int bytesread = 0;
char validKey[] = "2200536C73";   //Eric's Card
char validKey2[] = "36005B7320";  //Keychain
char validKey3[] = "2200776E9C";  //Chass' Card
char validKey4[] = "2100DFC897";  //Quarter Tag

// Guest Tags
char guestKey[] = "2200777B3A"; //Guest Tag
char guestKey2[] = "0415AFD41C";  //World Tag

Servo door;
#define rxPin 2
#define txPin 3
// RFID reader SOUT pin connected to Serial RX pin at 2400bps
SoftwareSerial RFID = SoftwareSerial(rxPin,txPin);

void setup()
{ 
  Serial.begin(9600);  // Hardware serial for Monitor 9600bps
  
  pinMode(6, OUTPUT);           // set pin to output
  digitalWrite(6, LOW);       // drive it low
  pinMode(5, INPUT);           // set pin to input
  digitalWrite(5, HIGH);       // turn on pullup resistors
  pinMode(11, OUTPUT);           // set pin to output
  digitalWrite(11, LOW);       // drive it low
  pinMode(12, INPUT);           // set pin to input
  digitalWrite(12, HIGH);       // turn on pullup resistors
  
  RFID.begin(2400);
}


void validKeySeq()
{
  door.attach(9);
  door.write(120);
  delay(1500);
  door.detach();
  delay(3500);
  while (digitalRead(5) == 1) {
    // wait for door close
  }
  delay(1000);
  door.attach(9);
  door.write(0);
  delay(1500);
  door.detach();
}


void loop()
{ 
  if((val = RFID.read()) == 10)
  {   // check for header 
    bytesread = 0; 
    while(bytesread < 10)
    {  // read 10 digit code 
      val = RFID.read(); 
      if((val == 10)||(val == 13))
      {  // if header or stop bytes before the 10 digit reading 
        break;                       // stop reading 
      } 
      code[bytesread] = val;         // add the digit           
      bytesread++;                   // ready to read next digit  
    } 

    if(bytesread == 10)
    {  // if 10 digit read is complete 
      Serial.print("RFID Read: ");   // possibly a good TAG
      
      Serial.println(code);            // print the TAG code
      
       // Valid TAG
       if (strncmp(code, validKey, 10) == 0 || strncmp(code, validKey2, 10) == 0 || strncmp(code, validKey3, 10) == 0 || strncmp(code, validKey4, 10) == 0)
         validKeySeq();
       else if (strncmp(code, guestKey, 10) == 0) {
         if (digitalRead(12) == 0)
           validKeySeq();
       }
       
    }
    bytesread = 0; 
    delay(250);                       // wait for a second
  } 
}
