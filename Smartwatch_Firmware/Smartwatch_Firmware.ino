#include <Encoder.h>
#include <LiquidCrystalFast.h>
#include <Time.h>
#include <LowPower_Teensy3.h>
#include "controls.h"
#include "states.h"
#include "flavortext.h"

// Contains code from TimeTeensy3 example

byte me;

#define TIMEZONE 0

byte state;
#define NUMBER_OF_APPS 8
byte apps[NUMBER_OF_APPS] = {
  LASER, FLASHLIGHT, TVBGONE, POWERPOINT, GLASS_DPAD, BREATHALYZER, LIFECOUNTER, SET_CLOCK};

TEENSY3_LP LP = TEENSY3_LP();
boolean isAwake;

#define TIME_BETWEEN_PHOTOCELL_READINGS 500 
#define PHOTOCELL A10
#define THRESHOLD_50 600
#define THRESHOLD_75 300
#define THRESHOLD_100 200
unsigned long lastBrightnessAdjustment;

unsigned long lastClockUpdate;
unsigned long lastClockAnimation;
byte currentClockAnimationFrame;
#define TOTAL_CLOCK_ANIMATION_FRAMES 24
#define TIME_BETWEEN_CLOCK_ANIMATION_FRAMES 25
byte currentFlavorText;
char currentClockString[21] = "XXXXXXXXXXXXXXXXXXXX";

byte appSwitcherPosition = 5;

boolean laserIsOn;
boolean flashlightIsOn;

int currentTvCode = 0;
const unsigned long tvCodeCount = 376;
const unsigned long timeBetweenTvCodes = 320;
unsigned long lastTvCodeSentTime;

char glassAddress[] = "F88FCA25F30C"; // TODO: Save and restore from EEPROM and update when it changes
boolean glassIsConnected;

int breathalyzerZero;
int breathalyzerMax = 1023;
int maxBreathalyzerReading;
#define MAXIMUM_BAC 0.2 // ?
#define BREATHALYZER_SAMPLES_LOG2 4
unsigned long breathalyzerHeatStart;
boolean breathalyzerHasHeated;
#define BREATHALYZER_HEAT_TIME 20000

int setHour;
int setMinute;
int setSecond;
boolean setIsAm;
int setMonth;
int setDay;
int setYear;

byte currentlySetting;
#define SETTING_HOUR 0
#define SETTING_MINUTE 1
#define SETTING_SECOND 2
#define SETTING_AM 3
#define SETTING_MONTH 4
#define SETTING_DAY 5
#define SETTING_YEAR 6
#define LAST_CLOCK_SETTING SETTING_YEAR

byte daysOfMonth[12] = {
  31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

byte lifecounterPlayer = 1;
byte playerLife[6] = {
  69, 20, 20, 0, 0, 0};

void setup() {
  Serial.begin(115200);
  Bluetooth.begin(115200);

  setSyncProvider(getTeensy3Time);

  randomSeed(analogRead(A13));

  configurePins();
  updateSwitches();
  updateSwitches();

  if (redSwitchIsUp) state = APPSWITCHER;
  else state = CLOCK;

  delay(1000);
  sleepBluetooth();

  handleStateStart();

  if (blackSwitchIsUp) wakeUp();
  else goToSleep();
}


void loop() {
  if (millis() - lastBrightnessAdjustment > TIME_BETWEEN_PHOTOCELL_READINGS) {
    int reading = analogRead(PHOTOCELL);

    Serial.println(reading);

    if (reading > THRESHOLD_50) {
      vfd.vfdDim(3);
      Serial.println("Brightness changed to level 0");
    }
    else if (reading > THRESHOLD_75) {
      vfd.vfdDim(2);
      Serial.println("Brightness changed to level 1");
    }
    else if (reading > THRESHOLD_100) {
      vfd.vfdDim(1);
      Serial.println("Brightness changed to level 2");
    }
    else {
      vfd.vfdDim(0);
      Serial.println("Brightness changed to level 3");
    }

    lastBrightnessAdjustment = millis();
  }

  updateSwitches();

  if (!blackSwitchIsUp) {
    goToSleep();
  }

  if (isAwake) {
    if (redSwitchIsUp) {
      if (!redSwitchWasUp) {
        Serial.println("Red switch flipped up");
        switchTo(APPSWITCHER);
      }
    }
    else {
      if (redSwitchWasUp) {
        Serial.println("Red switch flipped down");
        switchTo(CLOCK);
      }
    }
  }
  serialEvent3();

  updateState();
}


void serialEvent3() {
  while (Serial3.available()) {
    Serial.print((char)Serial3.read());
  }
}


void goToSleep() {
  Serial.println("Going to sleep");
  vfd.noDisplay();

  // TODO: handleStateSleep();

  flashlightWrite(flashlightIsOn);
  laserWrite(laserIsOn);

  isAwake = false;

  delay(100);

  attachInterrupt(BLACK_SWITCH, wakeUp, RISING);
  LP.Sleep();
  // Continues at interrupt vector wakeUp()
}


void wakeUp() {
  detachInterrupt(BLACK_SWITCH);
  delay(50);
  Serial.println("Waking up");

  setTime(getTeensy3Time());

  updateSwitches();
  updateSwitches();

  // TODO: handleStateWake();

  if (redSwitchIsUp) {
    if (state == CLOCK) switchTo(APPSWITCHER);
    currentClockAnimationFrame = TOTAL_CLOCK_ANIMATION_FRAMES;
  }
  else {
    currentClockAnimationFrame = 0;
    currentFlavorText = random(NUMBER_OF_FLAVOR_TEXTS);
    switchTo(CLOCK);
  }

  vfd.display();
  isAwake = true;
}


void switchTo(byte newState) {
  Serial.print("Switching to ");
  Serial.println(getAppName(newState));
  handleStateEnd();
  state = newState;
  handleStateStart();
}


void handleStateStart() {
  switch (state) {
  case CLOCK:
    vfd.clear();
    assembleClockString();
    if (currentClockAnimationFrame >= TOTAL_CLOCK_ANIMATION_FRAMES) {
      printDigitalClock();
      printFlavorText(currentFlavorText);
    }
    //sleepBluetooth();
    break;
  case APPSWITCHER:
    vfd.clear();
    vfd.setCursor(0, 0);
    vfd.print('>');
    vfd.print(getAppName(apps[appSwitcherPosition]));
    break;
  case LASER:
    //delay(BOUNCE_TIME);
    laserIsOn = !laserIsOn;
    laserWrite(laserIsOn);
    switchTo(APPSWITCHER);
    break;
  case FLASHLIGHT:
    //delay(BOUNCE_TIME);
    flashlightIsOn = !flashlightIsOn;
    flashlightWrite(flashlightIsOn);
    switchTo(APPSWITCHER);
    break;
  case TVBGONE:
    currentTvCode = 0;
    lastTvCodeSentTime = millis();
    vfd.clear();
    vfd.setCursor(0, 0);
    printTvBGone();
    break;
  case GLASS_DPAD:
    vfd.clear();
    vfd.setCursor(0, 0);
    vfd.println("Glass Control");
    vfd.print("Connecting...");

    wakeBluetooth();

    Bluetooth.write(0x00); // Kill any existing connections, just to be sure
    Bluetooth.print("$$$");
    //Bluetooth.print("W");
    delay(50);
    //Bluetooth.print("SR,");
    //Bluetooth.println(glassAddress);
    //delay(50);
    Bluetooth.println("S~,6");
    delay(50);
    Bluetooth.println("R,1");
    delay(500);
    Bluetooth.print("$$$");
    delay(50);
    Bluetooth.println("C");
    break;
  case BREATHALYZER:
    breathalyzerHeatStart = millis();
    breathalyzerHasHeated = false;
    digitalWrite(BREATHALYZER_POWER, HIGH);
    vfd.setCursor(0, 0);
    vfd.print("Heating...  Raw:    ");
    vfd.setCursor(0, 1);
    for (int i = 0; i < 20; i++) vfd.print(' ');
    break;
  case LIFECOUNTER:
    printLifecounter();
    break;
  case SET_CLOCK:
    setHour = hourFormat12();
    setMinute = minute();
    setSecond = second();
    setIsAm = hour() == hourFormat12();
    setMonth = month();
    setDay = day();
    setYear = year();
    currentlySetting = SETTING_HOUR;
    printClockSetting();
    break;
  default:
    switchTo(APPSWITCHER);
    break;
  }
}


void handleStateEnd() {
  switch (state) {
  case CLOCK:
    if (currentClockAnimationFrame > 0) currentClockAnimationFrame = TOTAL_CLOCK_ANIMATION_FRAMES;
  case APPSWITCHER:
    digitalWrite(LASER, laserIsOn);
    digitalWrite(FLASHLIGHT, flashlightIsOn);
    break;
  case GLASS_DPAD:
    Bluetooth.write(0x00);
    sleepBluetooth();

    //Bluetooth.write(0x00); // Disconnect

    //delay(50);
    //Bluetooth.print("$$$");
    //delay(50);
    //Bluetooth.print('Q');
    break;
  case BREATHALYZER:
    digitalWrite(BREATHALYZER_POWER, LOW);
    break;
  case SET_CLOCK:
    commitNewTime();
    break;
  default:
    break;
  }
}


void updateState() {
  int delta = 0; // Used for setting the clock

  switch (state) {
  case CLOCK:
    flashlightWrite(flashlightIsOn | tacSwitchUpIsPressed);
    laserWrite(laserIsOn | tacSwitchDownIsPressed);

    if (isAwake) {
      if (millis() - lastClockUpdate > 1000) { // Redraw clock every second
        assembleClockString();
        if (currentClockAnimationFrame >= TOTAL_CLOCK_ANIMATION_FRAMES) {
          printDigitalClock();
        }
        lastClockUpdate = millis();
      }

      if (currentClockAnimationFrame < TOTAL_CLOCK_ANIMATION_FRAMES &&
        millis() - lastClockAnimation >= TIME_BETWEEN_CLOCK_ANIMATION_FRAMES) {
        static byte cursorLocation = max(currentClockAnimationFrame - 2, 0);

        printDigitalClock(cursorLocation);

        if (cursorLocation < 20) {
          for (int i = cursorLocation; i < 20; i++) {
            vfd.write(i <= currentClockAnimationFrame ?
            random(0xA6, 0xDE) ://random(0x10, 0xFF) :
            0x20);
          }
        }

        cursorLocation = max(currentClockAnimationFrame - 3, 0);

        vfd.setCursor(0, 1);
        printFlavorText(currentFlavorText, cursorLocation);

        if (cursorLocation < 20) {
          //vfd.setCursor(cursorLocation, 1);
          for (int i = cursorLocation; i < 20; i++) {
            vfd.write(i <= currentClockAnimationFrame - 1 ?
            random(0xA6, 0xDE) ://random(0x10, 0xFF) :
            0x20);
          }
        }

        currentClockAnimationFrame++;
        lastClockAnimation = millis();
      }
    }
    break;

  case APPSWITCHER: 
    {
      byte lastAppSwitcherPosition = appSwitcherPosition;

      if (encoderDirection == CW) {
        if (++appSwitcherPosition == NUMBER_OF_APPS) appSwitcherPosition = 0;
      }
      else if (encoderDirection == CCW) {
        if (--appSwitcherPosition == 0xFF) appSwitcherPosition = NUMBER_OF_APPS - 1;
      }
      if (tacSwitchUpIsPressed) appSwitcherPosition = NUMBER_OF_APPS - 1;
      else if (tacSwitchDownIsPressed) appSwitcherPosition = 0;
      if (encoderIsClicked && !encoderWasClicked) switchTo(apps[appSwitcherPosition]);

      if (appSwitcherPosition != lastAppSwitcherPosition) {
        vfd.clear();
        vfd.setCursor(0, 0);
        vfd.print('>');
        vfd.print(getAppName(apps[appSwitcherPosition]));
      }
      break;
    }

  case TVBGONE:
    if (currentTvCode < tvCodeCount) {
      if (millis() - lastTvCodeSentTime > timeBetweenTvCodes) {
        currentTvCode++;
        vfd.clear();
        vfd.setCursor(0, 0);
        if (currentTvCode < tvCodeCount) printTvBGone();
        else {
          vfd.write(0x2A);
          vfd.print(" TV destroyed!");
        }
        lastTvCodeSentTime = millis();
      }
    }
    break;

  case GLASS_DPAD:
    if (glassIsConnected) {
      if (encoderDirection == CCW) Bluetooth.write(0x0B); // Left arrow
      else if (encoderDirection == CW) Bluetooth.write(0x07); // Right arrow

      //if (tacSwitchUpIsPressed && !tacSwitchUpWasPressed &&
      // millis() - lastTacSwitchUpChange > BOUNCE_TIME) // TODO: Pair mode
      if (tacSwitchDownIsPressed && !tacSwitchDownWasPressed /*&&
       millis() - lastTacSwitchDownChange > BOUNCE_TIME*/) Bluetooth.write(0x1B); // Escape
      if (encoderIsClicked && !encoderWasClicked /*&&
       millis() - lastEncoderClickChange > BOUNCE_TIME*/) Bluetooth.write(0x0A); // Enter
    }
    else {
      if (Bluetooth.available() && Bluetooth.read() == '>') { // Target is ">CONNECT"
        if (Bluetooth.read() == 'C' &&
          Bluetooth.read() == 'O' &&
          Bluetooth.read() == 'N') {
          glassIsConnected = true;
          vfd.setCursor(0, 1);
          vfd.print("Connected!          ");
        }
      }  
    }
    break;

  case BREATHALYZER:
    static int booze = analogRead(BREATHALYZER_OUTPUT);
    static double normalizedValue = 0;

    if (breathalyzerHasHeated) {
      if (tacSwitchUpIsPressed && !tacSwitchUpWasPressed) zeroBreathalyzer();
      if (tacSwitchDownIsPressed && !tacSwitchDownWasPressed) {
        maxBreathalyzerReading = 0;
        vfd.setCursor(0, 0);
        vfd.print("BAC: 0.00 Max: 0.00 ");
      }

      normalizedValue = constrain(mapDouble(booze, breathalyzerZero, breathalyzerMax, 0.0, 19), 0.0, 19);

      vfd.setCursor(5, 0);
      vfd.print(constrain(mapDouble(booze, breathalyzerZero, breathalyzerMax, 0.0, MAXIMUM_BAC), 0.0, MAXIMUM_BAC));

      if (booze > maxBreathalyzerReading) {
        maxBreathalyzerReading = booze;
        vfd.setCursor(15, 0);
        vfd.print(constrain(mapDouble(maxBreathalyzerReading, breathalyzerZero, breathalyzerMax, 0.0, MAXIMUM_BAC), 0.0, MAXIMUM_BAC));
      }
    }
    else {
      vfd.setCursor(16, 0);
      vfd.print(booze);
      if (booze < 1000) vfd.print(' ');
      if (booze < 100) vfd.print(' ');
      if (booze < 10) vfd.print(' ');

      normalizedValue = (double)(millis() - breathalyzerHeatStart) * 20.0 / (double)BREATHALYZER_HEAT_TIME;

      if (millis() - breathalyzerHeatStart > BREATHALYZER_HEAT_TIME) {
        breathalyzerHasHeated = true;
        zeroBreathalyzer();
        vfd.setCursor(0, 0);
        vfd.print("BAC: 0.00 Max: 0.00 ");
      }
    }

    vfd.setCursor(0, 1);
    //Serial.println(normalizedValue);
    for (int i = 0; i < 20; i++) {
      if (normalizedValue > i) {
        if (normalizedValue < i + 1) vfd.write(0x10 + ((int)((normalizedValue - floor(normalizedValue)) * 5) % 5));
        else vfd.write(0x14);
      }
      else {
        vfd.write(0x20);
      }
    }
    break;

  case LIFECOUNTER:
    if (!isAwake) break;

    if (encoderDirection == CCW) delta = 1;
    else if (encoderDirection == CW) delta = -1;

    if (delta != 0) {
      playerLife[lifecounterPlayer] += delta;
      printLifecounterPlayer(lifecounterPlayer);
      Serial.print("Player ");
      Serial.print(lifecounterPlayer);
      Serial.print(": ");
      Serial.println(playerLife[lifecounterPlayer]);
    }

    if (tacSwitchUpIsPressed && !tacSwitchUpWasPressed) {
      byte lastPlayer = lifecounterPlayer;
      lifecounterPlayer = lifecounterPlayer == 1 ? 5 : lifecounterPlayer - 1;
      printLifecounterPlayer(lastPlayer);
      printLifecounterPlayer(lifecounterPlayer);
      //delay(BOUNCE_TIME);
    }

    if ((tacSwitchDownIsPressed && !tacSwitchDownWasPressed) ||
      (encoderIsClicked && !encoderWasClicked)) {
      byte lastPlayer = lifecounterPlayer;
      lifecounterPlayer = lifecounterPlayer == 5 ? 1 : lifecounterPlayer + 1;
      printLifecounterPlayer(lastPlayer);
      printLifecounterPlayer(lifecounterPlayer);
      //delay(BOUNCE_TIME);
    }

    break;  

  case SET_CLOCK:
    if (!isAwake) break;

    if (encoderIsClicked && !encoderWasClicked) {
      currentlySetting++;  
      currentlySetting %= LAST_CLOCK_SETTING + 1;

      if (currentlySetting - 1 == SETTING_MONTH) setDay = min(setDay, daysOfMonth[setMonth - 1]);

      printClockSetting();
    }

    if (encoderDirection == CCW) delta = 1;
    else if (encoderDirection == CW) delta = -1;

    if (delta != 0) {
      switch (currentlySetting) {
      case SETTING_HOUR:
        setHour = (setHour + delta) % 12;
        if (setHour < 1) setHour = 12;
        break;
      case SETTING_MINUTE:
        setMinute = (setMinute + delta) % 60;
        if (setMinute < 0) setMinute = 59;
        break;
      case SETTING_SECOND:
        setSecond = (setSecond + delta) % 60;
        if (setSecond < 0) setSecond = 59;
        break;
      case SETTING_AM:
        setIsAm = !setIsAm;
        break;
      case SETTING_MONTH:
        setMonth = (setMonth + delta) % 12;
        if (setMonth < 1) setMonth = 12;
        break;
      case SETTING_DAY:
        setDay = (setDay + delta) % daysOfMonth[setMonth - 1];
        if (setDay < 1) setDay = daysOfMonth[setMonth - 1];
        // TODO: Handle leap years
        break;
      case SETTING_YEAR:
        setYear += delta;
        setYear = max(setYear, 1970);
        break;
      }

      printClockSetting();
    }
    break;

  default:
    break;
  }
}


String getAppName(byte stateToName) {
  switch (stateToName) {
  case CLOCK:
    return F("Clock");
  case APPSWITCHER:
    return F("App Switcher");
  case LASER:
    if (laserIsOn) return F("Deactivate Laser");
    else return F("Frickin' Laser");
  case FLASHLIGHT:
    if (flashlightIsOn) return F("Lights Out");
    else return F("Lights On");
  case TVBGONE:
    return F("TV-B-Gone");
  case POWERPOINT:
    return F("PowerPoint Remote");
  case GLASS_DPAD:
    return F("Glass Control");
  case BREATHALYZER:
    return F("Breathalyzer");
  case LIFECOUNTER:
    return F("MTG Life Counter");
  case SET_CLOCK:
    return F("Set Clock");
  default:
    return F("Error");
  }
}


void commitNewTime() {
  TimeElements tm;

  tm.Second = setSecond;
  tm.Minute = setMinute;
  tm.Hour = setIsAm ? setHour : setHour + 12;
  tm.Day = setDay;
  tm.Month = setMonth;
  tm.Year = setYear - 1970;

  Teensy3Clock.set(makeTime(tm));
  setTime(makeTime(tm));

  setHour = setMinute = setSecond = setDay = setMonth = setYear = currentlySetting = 0;
}


void printClockSetting() {
  vfd.clear();

  vfd.setCursor(0, 0);
  vfd.print(setHour);
  printDigits(setMinute);
  printDigits(setSecond);
  vfd.print(" ");
  vfd.print(setIsAm ? "AM" : "PM");
  vfd.print(" ");
  vfd.print(setMonth);
  vfd.print('/');
  vfd.print(setDay);
  vfd.print('/');
  vfd.print(setYear % 100); 

  byte caretLocation = 0;
  if (currentlySetting > SETTING_HOUR) {
    caretLocation += 2;
    if (setHour > 9) caretLocation++;
  }
  if (currentlySetting > SETTING_MINUTE) {
    caretLocation += 3;
  }
  if (currentlySetting > SETTING_SECOND) {
    caretLocation += 3;
  }
  if (currentlySetting > SETTING_AM) {
    caretLocation += 3;
  }
  if (currentlySetting > SETTING_MONTH) {
    caretLocation += 2;
    if (setMonth > 9) caretLocation++;
  }
  if (currentlySetting > SETTING_DAY) {
    caretLocation += 2;
    if (setDay > 9) caretLocation++;
  }

  vfd.setCursor(caretLocation, 1);
  vfd.write(0x1F);
}


void assembleClockString() {
  String output = "";
  output += hourFormat12();
  output += ':';
  if (minute() < 10) output += '0';
  output += minute();
  output += ':';
  if (second() < 10) output += '0';
  output += second();
  output += hourFormat12() == hour() ? " AM " : " PM ";
  output += month();
  output += '/';
  output += day();
  output += '/';
  output += year() % 100;

  while (output.length() < 20) {
    output += ' ';
  }

  output.toCharArray(currentClockString, 21);

  Serial.println(currentClockString);
}


void printDigitalClock() {
  printDigitalClock(20);
}


void printDigitalClock(byte count) {
  vfd.setCursor(0, 0);
  for (int i = 0; i < count; i++) {
    if (currentClockString[i] != ' ' && currentClockString[i] != 0x00 && random(20) > count) vfd.write(random(0xA6, 0xDE));
    else vfd.print(currentClockString[i]);
  }
}


void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  vfd.print(":");
  if(digits < 10)
    vfd.print('0');
  vfd.print(digits);
}


void printFlavorText(byte index) {
  printFlavorText(index, 20);
}


void printFlavorText(byte index, byte count) {
  vfd.setCursor(0, 1);
  for (int i = 0; i < count; i++) {
    if (flavorTexts[index][i] != ' ' && flavorTexts[index][i] != 0x00 && random(20) > count) vfd.write(random(0xA6, 0xDE));
    else if (flavorTexts[index][i] == 0x00) vfd.print(' ');
    else vfd.print(flavorTexts[index][i]);
  }
}


void printTvBGone() {
  if (currentTvCode % 10 <= 8) {
    vfd.write(0x10 + currentTvCode % 10);
  }
  else {
    vfd.write(0x20); 
  }

  vfd.print(" Sending \"off\" #");
  vfd.print(currentTvCode);
}

void printLifecounter() {
  vfd.clear();
  for (int i = 1; i <= 5; i++) {
    printLifecounterPlayer(i);
  }
}

void printLifecounterPlayer(byte player) {
  switch(player) {
  case 1:
    vfd.setCursor(0, 0);
    break;
  case 2:
    vfd.setCursor(0, 1);
    break;
  case 3:
    vfd.setCursor(5, 1);
    break;
  case 4:
    vfd.setCursor(10, 1);
    break;
  case 5:
    vfd.setCursor(15, 1);
    break;
  default:
    return;
  }

  vfd.write(lifecounterPlayer == player ? 0x1D : 0x20);
  vfd.print(playerLife[player]);
  if (playerLife[player] < 10) vfd.write(0x20);
  if (playerLife[player] < 100) vfd.write(0x20);
  vfd.write(0x20);
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}


void sleepBluetooth() {
  Bluetooth.println("---");
  delay(100);
  Bluetooth.print("$$$");
  delay(100);
  //Bluetooth.println();
  Bluetooth.println("SI,0000"); // Disable discoverability
  delay(50);
  Bluetooth.println("SJ,0000"); // Disable connectability
  delay(50);
  Bluetooth.println("S%,1000"); // Set all I/O pins hi-Z
  delay(50);
  Bluetooth.println("S|,0120"); // Sleep for 95% of the time
  delay(50);
  Bluetooth.println("R,1"); // Reset to enact
  delay(500);
  //Bluetooth.println("Q"); // Quiet mode - nondiscoverable and non-connectable
}


void wakeBluetooth() {
  //Bluetooth.println("W"); // Wake RN from quiet mode
  Bluetooth.println("---");
  delay(100);
  Bluetooth.print("$$$");
  delay(100);
  //Bluetooth.println();
  Bluetooth.println("SI,0200"); // Enable discoverability
  delay(50);
  Bluetooth.println("SJ,0200"); // Enable connectability
  delay(50);
  //Bluetooth.println("S%,1000"); // Set all I/O pins hi-Z
  //delay(50);
  Bluetooth.println("S|,0000"); // Constantly look for a connection
  delay(50);
  Bluetooth.println("R,1"); // Reset to enact
  delay(500);
}




void zeroBreathalyzer() {
  breathalyzerZero = 0;
  for (int i = 0; i < pow(2, BREATHALYZER_SAMPLES_LOG2); i++) {
    breathalyzerZero += analogRead(BREATHALYZER_OUTPUT);
  }
  breathalyzerZero >>= BREATHALYZER_SAMPLES_LOG2;
  Serial.print("Breathalyzer zero: ");
  Serial.println(breathalyzerZero);
}


double mapDouble(double x, double in_min, double in_max, double out_min, double out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

























