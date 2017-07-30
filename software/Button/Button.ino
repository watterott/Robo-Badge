/*
  Blink
  Turns on and off a LED, when pressing a pushbutton

  Board: ATtiny841 (8MHz)
  Core:  Standard Arduino (pin map from David A. Mellis)
 */

// pin map from David A. Mellis
int ledPin1   = 8; // PB2 = D8 (LED low active)
int ledPin2   = 3; // PA3 = D3 (LED high active)
int buttonPin = 0; // PA0 = D0 (low active)
// pin map from Spence Konde
//int ledPin1   =  2; // PB2 = D2 (LED low active)
//int ledPin2   =  7; // PA3 = D7 (LED high active)
//int buttonPin = 10; // PA0 = D10 (low active)

// the setup function runs once when you press reset or power the board
void setup()
{
  // initialize digital pins as outputs and set to LOW
  pinMode(ledPin1, OUTPUT);
  digitalWrite(ledPin1, HIGH); // low active
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, LOW); // high active
  
  // initialize digital pin as an input with pull-up
  pinMode(buttonPin, INPUT_PULLUP);
  PUEA |= _BV(0); // PA0/D0 pull-up on
}

// the loop function runs over and over again forever
void loop()
{
  // variable for reading the pushbutton status
  int buttonState;

  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed
  if(buttonState == LOW) // LOW -> pressed
  {
    // turn the LEDs on
    digitalWrite(ledPin1, LOW); // low active
    digitalWrite(ledPin2, HIGH); // high active
  }
  else // not pressed
  {
    // turn the LEDs off
    digitalWrite(ledPin1, HIGH); // low active
    digitalWrite(ledPin2, LOW); // high active
  }
}
