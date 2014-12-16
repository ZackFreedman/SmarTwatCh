#include <Arduino.h>
#include <Encoder.h>
#include <LiquidCrystalFast.h>

#define BOUNCE_TIME 50
#define ENCODER_CLICK_BOUNCE_TIME 500

#define NONE 0
#define CCW 1
#define CW 2

#define VFD_RS 18
#define VFD_EN 19
#define VFD_D4 22
#define VFD_D5 20
#define VFD_D6 23
#define VFD_D7 21
LiquidCrystalFast vfd(VFD_RS, VFD_EN,
VFD_D4, VFD_D5, VFD_D6, VFD_D7);

#define BREATHALYZER_POWER 12
#define BREATHALYZER_OUTPUT 17

#define TACSWITCH_UP 5
boolean tacSwitchUpIsPressed, tacSwitchUpWasPressed;
unsigned long lastTacSwitchUpChange;
#define TACSWITCH_DOWN 6
boolean tacSwitchDownIsPressed, tacSwitchDownWasPressed;
unsigned long lastTacSwitchDownChange;

#define ENCODER_A 10
#define ENCODER_B 9
Encoder encoder(ENCODER_A, ENCODER_B);
long encoderBenchmark;
byte encoderDirection;
#define ENCODER_CLICK 11
boolean encoderIsClicked, encoderWasClicked;
boolean lastEncoderClickChange;

#define BLACK_SWITCH 4
boolean blackSwitchIsUp, blackSwitchWasUp;
unsigned long lastBlackSwitchChange;
#define RED_SWITCH 3
boolean redSwitchIsUp, redSwitchWasUp;
unsigned long lastRedSwitchChange;

#define Bluetooth Serial3

const byte LASER = 14;
const byte FLASHLIGHT = 16;
const byte BLASTER = 15;

void configurePins() {
  vfd.begin(20, 2);
  vfd.noDisplay();

  pinMode(BREATHALYZER_POWER, OUTPUT);

  pinMode(TACSWITCH_UP, INPUT_PULLUP);
  pinMode(TACSWITCH_DOWN, INPUT_PULLUP);

  pinMode(ENCODER_CLICK, INPUT_PULLUP);

  pinMode(BLACK_SWITCH, INPUT_PULLUP);
  pinMode(RED_SWITCH, INPUT_PULLUP);

  pinMode(LASER, OUTPUT);
  pinMode(FLASHLIGHT, OUTPUT);
  pinMode(BLASTER, OUTPUT);
}

void updateSwitches() {
  unsigned long startingMillis = millis();
  
  tacSwitchUpWasPressed = tacSwitchUpIsPressed;
  tacSwitchDownWasPressed = tacSwitchDownIsPressed;
  encoderWasClicked = encoderIsClicked;
  blackSwitchWasUp = blackSwitchIsUp;
  redSwitchWasUp = redSwitchIsUp;
  //redSwitchWasUp = false/;

  static long encoderReading = encoder.read();

  if (startingMillis - lastTacSwitchUpChange > BOUNCE_TIME) 
    tacSwitchUpIsPressed = !digitalRead(TACSWITCH_UP);
    
  if (startingMillis - lastTacSwitchDownChange > BOUNCE_TIME) 
    tacSwitchDownIsPressed = !digitalRead(TACSWITCH_DOWN);
    
  if (startingMillis - lastEncoderClickChange > ENCODER_CLICK_BOUNCE_TIME) 
    encoderIsClicked = !digitalRead(ENCODER_CLICK);
    
  if (startingMillis - lastBlackSwitchChange > BOUNCE_TIME) 
    blackSwitchIsUp = digitalRead(BLACK_SWITCH);

  pinMode(RED_SWITCH, INPUT_PULLUP); // TODO: Figure out what keeps flipping it back to an INPUT
  
  if (startingMillis - lastRedSwitchChange > BOUNCE_TIME) 
    redSwitchIsUp = digitalRead(RED_SWITCH);

  if (tacSwitchUpWasPressed != tacSwitchUpIsPressed) lastTacSwitchUpChange = startingMillis;
  if (tacSwitchDownWasPressed != tacSwitchDownIsPressed) lastTacSwitchDownChange = startingMillis;
  if (encoderWasClicked != encoderIsClicked) {
    lastEncoderClickChange = startingMillis;
    encoderBenchmark = encoderReading;
  }
  if (redSwitchWasUp != redSwitchIsUp) {
    lastRedSwitchChange = startingMillis;
    encoderBenchmark = encoderReading;
  }
  if (blackSwitchWasUp != blackSwitchIsUp) {
    lastBlackSwitchChange = startingMillis;
    encoderBenchmark = encoderReading;
  }

  /*
  if (encoderReading != encoderBenchmark) {
   Serial.print(encoderReading);
   Serial.print(" --> ");
   Serial.println(encoderBenchmark);
   }
   */

  if (abs(encoderReading - encoderBenchmark) >= 4) {
    if (encoderReading > encoderBenchmark) encoderDirection = CCW;
    else encoderDirection = CW; 

    encoderBenchmark = encoderReading;
  }
  else encoderDirection = NONE;
}

void laserWrite(boolean input) {
  digitalWrite(LASER, input);
}

void flashlightWrite(boolean input) {
  digitalWrite(FLASHLIGHT, input);
}




