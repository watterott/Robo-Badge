/*
  TV-Off
  A TV-B-Gone firmware for Robo-Badge.

  Based on TV-B-Gone Arduino port by Ken Shirriff and Gabriel Staples
  https://github.com/shirriff/Arduino-TV-B-Gone
  Original TV-B-Gone Firmware by Mitch Altman & Limor Fried
  http://www.tvbgone.com

  This project transmits a bunch of TV POWER codes, one right after the other,
  with a pause in between each.  (To have a visible indication that it is
  transmitting, it also pulses a visible LED once each time a POWER code is
  transmitted.) That is all TV-B-Gone does.  The tricky part of TV-B-Gone
  was collecting all of the POWER codes, and getting rid of the duplicates and
  near-duplicates (because if there is a duplicate, then one POWER code will
  turn a TV off, and the duplicate will turn it on again (which we certainly
  do not want). I have compiled the most popular codes with the
  duplicates eliminated, both for North America (which is the same as Asia, as
  far as POWER codes are concerned -- even though much of Asia USES PAL video)
  and for Europe (which works for Australia, New Zealand, the Middle East, and
  other parts of the world that use PAL video).

  Hardware:
  PA3/D3 (OC0B/TOCC2) - IR-LED
  PB2/D8 - Normal LED
  PA0/D0 - Trigger Switch
  PA4/D4 - Optional Region Switch

  Board: ATtiny841 (8MHz)
  Core:  Standard Arduino (pin map from David A. Mellis)
*/

// The TV-B-Gone for Arduino can use either the EU (European Union) or the NA (North America) database of POWER CODES
// EU is for Europe, Middle East, Australia, New Zealand, and some countries in Africa and South America
// NA is for North America, Asia, and the rest of the world not covered by EU
#define REGION EU        // EU or NA, region when no region switch is available

// What pins do what
#define IRLED 3          // the IR sender LED
#define LED 8            // LED indicator pin (usually 13)
#define TRIGGER 0        // the button pin
#define BUTTON_PRESSED 0 // LOW (0) = button pressed
//#define REGIONSWITCH 4   // HIGH (1) = NA, LOW (0) = EU; Pin 5 (REGIONSWITCH) is HIGH (via in input pullup resistor) for North America, or you (the user) must wire it to ground to set the codes for Europe.

#define NA 1 // set by a HIGH on REGIONSWITCH pin
#define EU 0 // set by a LOW on REGIONSWITCH pin

#ifdef REGIONSWITCH // region switch available
# define NA_CODES
# define EU_CODES
#else // no region switch
# if REGION == NA
#  define NA_CODES
# else
#  define EU_CODES
# endif
#endif

#include "WORLD_IR_CODES.h"

// function protopypes
void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code);
uint8_t read_bits(uint8_t count);
void sendAllCodes(void);
void delay_ten_us(uint16_t us);
void quickflashLED(void);
void quickflashLEDx(uint8_t x);

uint8_t region;

void setup()   
{
  #if DEBUG > 0
    Serial.begin(9600);
  #endif

  // reset timer0
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0 = 0;
  TIFR0 = 0;
  TIMSK0 = 0;
  OCR0A = 0;
  OCR0B = 0;
  #if defined(AVR_ATTINY841) || defined(__AVR_ATtiny841__)
    TOCPMSA0 = (0<<TOCC2S1) | (0<<TOCC2S0); //TOCC2=OC0B
    TOCPMSA1 = 0;
    TOCPMCOE = 0;
  #endif

  // set pin modes/states
  digitalWrite(LED, HIGH);
  digitalWrite(IRLED, LOW);
  pinMode(LED, OUTPUT);
  pinMode(IRLED, OUTPUT);
  #ifdef REGIONSWITCH
    pinMode(REGIONSWITCH, INPUT_PULLUP);
    PUEA |= _BV(4);
  #endif
  #ifdef TRIGGER
    pinMode(TRIGGER, INPUT_PULLUP);
    PUEA |= _BV(0);
  #endif

  delay_ten_us(5000); // 50ms (5000x10 us) delay: let everything settle for a bit

  // determine region
  #ifdef REGIONSWITCH
    if (digitalRead(REGIONSWITCH)) {
      region = NA;
      DEBUGP(putstring_nl("NA"));
    }
    else {
      region = EU;
      DEBUGP(putstring_nl("EU"));
    }
  #else
    region = REGION;
  #endif

  // Debug output: indicate how big our database is
  #ifdef NA_CODES
    DEBUGP(putstring("\n\rNA Codesize: ");
           putnum_ud(num_NAcodes));
  #endif
  #ifdef EU_CODES
    DEBUGP(putstring("\n\rEU Codesize: ");
           putnum_ud(num_EUcodes));
  #endif

  // Tell the user what region we're in  - 3 flashes is NA, 6 is EU
  quickflashLEDx((region+1)*3);
}

#ifdef NA_CODES
extern const IrCode* const NApowerCodes[] PROGMEM;
extern uint8_t num_NAcodes;
#endif
#ifdef EU_CODES
extern const IrCode* const EUpowerCodes[] PROGMEM;
extern uint8_t num_EUcodes;
#endif
uint16_t ontime, offtime;
uint8_t i, num_codes;

// This function is the 'workhorse' of transmitting IR codes.
// Given the on and off times, it turns on the PWM output on and off
// to generate one 'pair' from a long code. Each code has ~50 pairs!
void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code)
{
  // start Timer0 outputting the carrier frequency to IR emitters
  TCNT0 = 0; // reset the timers so they are aligned
  TIFR0 = 0; // clean out the timer flags

  if (PWM_code) {
    // set up timer 0: CTC, Top=OCR0A, no prescaling
    TCCR0A = _BV(COM0B0) | _BV(WGM01);
    TCCR0B = _BV(CS00);
    #if defined(AVR_ATTINY841) || defined(__AVR_ATtiny841__)
      TOCPMCOE |= (1<<TOCC2OE);
    #endif
  }
  else {
    // However some codes dont use PWM in which case we just turn the IR
    // LED on for the period of time.
    digitalWrite(IRLED, HIGH);
  }

  // Now we wait, allowing the PWM hardware to pulse out the carrier
  // frequency for the specified 'on' time
  delay_ten_us(ontime);

  // Now we have to turn it off so disable the PWM output
  TCCR0A = 0;
  TCCR0B = 0;
  #if defined(AVR_ATTINY841) || defined(__AVR_ATtiny841__)
    TOCPMCOE &= ~(1<<TOCC2OE);
  #endif
  // And make sure that the IR LED is off too (since the PWM may have
  // been stopped while the LED is on!)
  digitalWrite(IRLED, LOW);

  // Now we wait for the specified 'off' time
  delay_ten_us(offtime);
}

/*
  This is kind of a strange but very useful helper function
  Because we are using compression, we index to the timer table
  not with a full 8-bit byte (which is wasteful) but 2 or 3 bits.
  Once code_ptr is set up to point to the right part of memory,
  this function will let us read 'count' bits at a time which
  it does by reading a byte into 'bits_r' and then buffering it.
 */

uint8_t bitsleft_r=0;
uint8_t bits_r=0;
PGM_P code_ptr;

// we cant read more than 8 bits at a time so dont try!
uint8_t read_bits(uint8_t count)
{
  uint8_t i;
  uint8_t tmp=0;

  // we need to read back count bytes
  for (i=0; i<count; i++) {
    // check if the 8-bit buffer we have has run out
    if (bitsleft_r == 0) {
      // in which case we read a new byte in
      bits_r = pgm_read_byte(code_ptr++);
      // and reset the buffer size (8 bites in a byte)
      bitsleft_r = 8;
    }
    // remove one bit
    bitsleft_r--;
    // and shift it off of the end of 'bits_r'
    tmp |= (((bits_r >> (bitsleft_r)) & 1) << (count-1-i));
  }
  // return the selected bits in the LSB part of tmp
  return tmp;
}

/*
  The C compiler creates code that will transfer all constants into RAM when
  the microcontroller resets.  Since this firmware has a table (powerCodes)
  that is too large to transfer into RAM, the C compiler needs to be told to
  keep it in program memory space.  This is accomplished by the macro PROGMEM
  (this is used in the definition for powerCodes).  Since the C compiler assumes
  that constants are in RAM, rather than in program memory, when accessing
  powerCodes, we need to use the pgm_read_word() and pgm_read_byte macros, and
  we need to use powerCodes as an address.  This is done with PGM_P, defined
  below.
  For example, when we start a new powerCode, we first point to it with the
  following statement:
  PGM_P thecode_p = pgm_read_word(powerCodes+i);
  The next read from the powerCode is a byte that indicates the carrier
  frequency, read as follows:
  const uint8_t freq = pgm_read_byte(code_ptr++);
  After that is a byte that tells us how many 'onTime/offTime' pairs we have:
  const uint8_t numpairs = pgm_read_byte(code_ptr++);
  The next byte tells us the compression method. Since we are going to use a
  timing table to keep track of how to pulse the LED, and the tables are
  pretty short (usually only 4-8 entries), we can index into the table with only
  2 to 4 bits. Once we know the bit-packing-size we can decode the pairs
  const uint8_t bitcompression = pgm_read_byte(code_ptr++);
  Subsequent reads from the powerCode are n bits (same as the packing size)
  that index into another table in ROM that actually stores the on/off times
  const PGM_P time_ptr = (PGM_P)pgm_read_word(code_ptr);
*/

void sendAllCodes(void) 
{
  bool endingEarly = false; // will be set to true if the user presses the button during code-sending 

  // determine region from REGIONSWITCH: 1 = NA, 0 = EU (defined in main.h)
  #ifdef REGIONSWITCH
    if (digitalRead(REGIONSWITCH)) {
      region = NA;
      num_codes = num_NAcodes;
    }
    else {
      region = EU;
      num_codes = num_EUcodes;
    }
  #else
    #ifdef NA_CODES
      num_codes = num_NAcodes;
    #else
      num_codes = num_EUcodes;
    #endif
  #endif

  // for every POWER code in our collection
  for (i=0 ; i<num_codes; i++) 
  {
    PGM_P data_ptr;

    // print out the code # we are about to transmit
    DEBUGP(putstring("\n\r\n\rCode #: ");
           putnum_ud(i));

    // point to next POWER code, from the right database
    #ifdef REGIONSWITCH
      if (region == NA) {
        data_ptr = (PGM_P)pgm_read_word(NApowerCodes+i);
      }
      else {
        data_ptr = (PGM_P)pgm_read_word(EUpowerCodes+i);
      }
    #else
      #ifdef NA_CODES
        data_ptr = (PGM_P)pgm_read_word(NApowerCodes+i);
      #else
        data_ptr = (PGM_P)pgm_read_word(EUpowerCodes+i);
      #endif
    #endif

    // print out the address in ROM memory we're reading
    DEBUGP(putstring("\n\rAddr: ");
           putnum_uh((uint16_t)data_ptr));

    // read the carrier frequency from the first byte of code structure
    const uint8_t freq = pgm_read_byte(data_ptr++);
    // set OCR for Timer1 to output this POWER code's carrier frequency
    OCR0A = freq; 
    OCR0B = freq; 

    // print out the frequency of the carrier and the PWM settings
    DEBUGP(putstring("\n\rOCR1: ");
           putnum_ud(freq));
    DEBUGP(uint16_t x = (freq+1) * 2;
           putstring("\n\rFreq: ");
           putnum_ud(F_CPU/x));

    // get the number of pairs, the second byte from the code struct
    const uint8_t numpairs = pgm_read_byte(data_ptr++);
    DEBUGP(putstring("\n\rOn/off pairs: ");
           putnum_ud(numpairs));

    // Get the number of bits we use to index into the timer table
    // This is the third byte of the structure
    const uint8_t bitcompression = pgm_read_byte(data_ptr++);
    DEBUGP(putstring("\n\rCompression: ");
           putnum_ud(bitcompression);
           putstring("\n\r"));

    // Get pointer (address in memory) to pulse-times table
    // The address is 16-bits (2 byte, 1 word)
    PGM_P time_ptr = (PGM_P)pgm_read_word(data_ptr);
    data_ptr+=2;
    code_ptr = (PGM_P)pgm_read_word(data_ptr);

    // Transmit all codeElements for this POWER code
    // (a codeElement is an onTime and an offTime)
    // transmitting onTime means pulsing the IR emitters at the carrier
    // frequency for the length of time specified in onTime
    // transmitting offTime means no output from the IR emitters for the
    // length of time specified in offTime

    // DEVELOPMENTAL TESTING
    #if 0
      // print out all of the pulse pairs
      for (uint8_t k=0; k<numpairs; k++) {
        uint8_t ti;
        ti = (read_bits(bitcompression)) * 4;
        // read the onTime and offTime from the program memory
        ontime = pgm_read_word(time_ptr+ti);
        offtime = pgm_read_word(time_ptr+ti+2);
        DEBUGP(putstring("\n\rti = ");
               putnum_ud(ti>>2);
               putstring("\tPair = ");
               putnum_ud(ontime));
        DEBUGP(putstring("\t");
               putnum_ud(offtime));
      }
      continue;
    #endif

    // for EACH pair in this code....
    cli(); // disable interrupts
    for (uint8_t k=0; k<numpairs; k++) {
      uint16_t ti;

      // Read the next 'n' bits as indicated by the compression variable
      // The multiply by 4 because there are 2 timing numbers per pair
      // and each timing number is one word long, so 4 bytes total!
      ti = (read_bits(bitcompression)) * 4;

      // read the onTime and offTime from the program memory
      ontime = pgm_read_word(time_ptr+ti);  // read word 1 - ontime
      offtime = pgm_read_word(time_ptr+ti+2);  // read word 2 - offtime
      // transmit this codeElement (ontime and offtime)
      xmitCodeElement(ontime, offtime, (freq!=0));
    }
    sei(); // enable interrupts

    // flush remaining bits, so that next code starts
    // with a fresh set of 8 bits.
    bitsleft_r=0;

    // visible indication that a code has been output.
    quickflashLED();

    // delay 205 milliseconds before transmitting next POWER code
    delay_ten_us(20500);

    // if user is pushing (holding down) TRIGGER button, stop transmission early 
    #ifdef TRIGGER
      if (digitalRead(TRIGGER) == BUTTON_PRESSED) 
      {
        endingEarly = true;
        delay_ten_us(50000); //500ms delay 
        quickflashLEDx(4);
        // pause for ~1.3 sec to give the user time to release the button so that the code sequence won't immediately start again.
        delay_ten_us(65500); //650ms delay 
        delay_ten_us(65500); //650ms delay 
        break; //exit the POWER code "for" loop
      }
    #endif
  } // end of POWER code for loop

  if (endingEarly == false)
  {
    // pause for ~1.3 sec, then flash the visible LED 8 times to indicate that we're done
    delay_ten_us(65500); //650ms delay 
    delay_ten_us(65500); //650ms delay 
    quickflashLEDx(8);
  }
}

void loop() 
{
  // Super "ghetto" (but decent enough for this application) button debouncing:
  // if the user pushes the Trigger button, then wait a while to let the button stop bouncing, then start transmission of all POWER codes
  #ifdef TRIGGER
    if (digitalRead(TRIGGER) == BUTTON_PRESSED) 
    {
     delay_ten_us(50000);  // delay 500ms to give the user time to release the button before the code sequence begins
  #endif
     sendAllCodes();
  #ifdef TRIGGER
    }
  #endif
  #ifdef LED
    static uint16_t led=0;
    if(++led == 0x1000)
    {
      digitalWrite(LED, LOW);
      delay_ten_us(3000); // 30 ms ON-time delay
      digitalWrite(LED, HIGH);
    }
  #endif
}

// This function delays the specified number of 10 microseconds
// it is 'hardcoded' and is calibrated by adjusting DELAY_CNT
// in main.h Unless you are changing the crystal from 8MHz, dont
// mess with this.
// due to uint16_t datatype, max delay is 65535 tens of microseconds, or 655350 us, or 655.350 ms. 
// NB: DELAY_CNT has been increased in main.h from 11 to 25 (last I checked) in order to allow this function
// to work properly with 16MHz Arduinos now (instead of 8MHz).
void delay_ten_us(uint16_t us)
{
  uint8_t timer;
  while (us != 0) {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer=0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}

// This function quickly pulses the visible LED
// This will indicate to the user that a code is being transmitted
void quickflashLED(void)
{
  digitalWrite(LED, LOW);
  delay_ten_us(3000); // 30 ms ON-time delay
  digitalWrite(LED, HIGH);
}

// This function just flashes the visible LED a couple times, used to
// tell the user what region is selected
void quickflashLEDx(uint8_t x)
{
  quickflashLED();
  while (--x) {
    delay_ten_us(25000); // 250 ms OFF-time delay between flashes
    quickflashLED();
  }
}
