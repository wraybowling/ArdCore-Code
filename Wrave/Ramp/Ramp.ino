/**
* Board Type:   ArdCore Eurorack Module / Arduino UNO 
* Description:  Buchla-style ramp generator
*/

// Analog Inputs
const int A_trapezoidalAmount = 0; // overshoots the max value for a sustain effect
const int A_cycling = 1; // makes the ramp loop past 12:00
const int A_riseSpeed = 2;
const int A_fallSpeed = 3;
// Input Pins
const int clockPin = 2; // start a new ramp
// Output Pins
const int digitalOutPin[2] = {3, 4};
const int dacFirstPin = 5; //5 through 12

// Options
const int rampFromCurrentValue = 1;
const float smoothingMillis = 50;

// Programmer's Notes
//
// a. look for a clock pulse
// b. ramp up using knob as speed
// c. ramp down using other knob as speed
// d. use fixed knob as shape selection
// e. use other fixed knob to select which bit comes out of D1
// f. match clock pulse with stored vars
//
// The range of the ramp is double what the output can actually handle
// so if you send rapid pulses, it's kind of like having a sustain

// Globals
const int triggerTime = 25;
float prevOutput = 0.0;
float thisOutput = 0.0; // could also be a byte apparently?
volatile int clockState = LOW;
long prevMillis = 0;
int rising = 0;

void clockInterrupt() { clockState = HIGH; }

void setup() {
//  Serial.begin(9600);

  // a. clock input will interrupt the program
  pinMode(clockPin, INPUT);
  attachInterrupt(0, clockInterrupt, RISING);
  
  // 2 digital outputs
  for (int i=0; i<2; i++) {
    pinMode(digitalOutPin[i], OUTPUT);
    digitalWrite(digitalOutPin[i], LOW);
  }
  
  // 8-bit DAC output
  for (int i=0; i<8; i++) {
    pinMode(dacFirstPin+i, OUTPUT);
    digitalWrite(dacFirstPin+i, LOW);
  }
}

void loop() {
  const float trapezoidalAmount = analogRead(A_trapezoidalAmount) / 1023.0 * 10;
  
  if(clockState == HIGH){
    clockState = LOW;
    rising = 1;
    if(thisOutput > 1) thisOutput = 1;
  }
  
  if(rising) {
    // b. ramp up
    double riseSpeed = sq(analogRead(A_riseSpeed) / 1023.0) / 10;
    // let the current value get up to twice the range
    thisOutput = min(1.0 + trapezoidalAmount, thisOutput + riseSpeed);
    // just the left LED
    digitalWrite(digitalOutPin[0], HIGH);
    digitalWrite(digitalOutPin[1], LOW);
    // switch off rising if we hit the top
    if(thisOutput >= trapezoidalAmount + 1.0) rising = 0;
  }
  
  if(!rising) {
    // c. ramp down
    double fallSpeed = sq(analogRead(A_fallSpeed) / 1023.0) / 10;
    thisOutput = max(0.0, thisOutput - fallSpeed);
    // just the right LED
    digitalWrite(digitalOutPin[0], LOW);
    digitalWrite(digitalOutPin[1], HIGH);
    // switch off rising if we hit the bottom and self-cyling is on
    if(thisOutput <= 0.0 && analogRead(A_cycling) > 512) rising = 1;
  }
  
  float clampedOutput = min(thisOutput, 1.0);
  byte voltage = byte(clampedOutput * 255);
  dacOutput(voltage);
}

void dacOutput(byte v)
{
  PORTB = (PORTB & B11100000) | (v >> 3);
  PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}
