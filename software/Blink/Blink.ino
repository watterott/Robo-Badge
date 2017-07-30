/*
  Blink
  Turns on an LED for one second, then off for one second, repeatedly.

  Board: ATtiny841 (8MHz)
  Core:  Standard Arduino (pin map from David A. Mellis)
 */

// pin map from David A. Mellis
int ledPin1 = 8; // PB2 = D8 (LED low active)
int ledPin2 = 3; // PA3 = D3 (LED high active)
// pin map from Spence Konde
//int ledPin1 = 2; // PB2 = D2 (LED low active)
//int ledPin2 = 7; // PA3 = D7 (LED high active)

// the setup function runs once when you press reset or power the board
void setup()
{
  // initialize digital pins as outputs and set to LOW
  pinMode(ledPin1, OUTPUT);
  digitalWrite(ledPin1, LOW);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, LOW);
}

// the loop function runs over and over again forever
void loop()
{
  // turn the LEDs on
  digitalWrite(ledPin1, LOW); // low active
  digitalWrite(ledPin2, HIGH); // high active

  // wait for a second
  delay(1000);

  // turn the LEDs off
  digitalWrite(ledPin1, HIGH); // low active
  digitalWrite(ledPin2, LOW); // high active

  // wait for a second
  delay(1000);
}
