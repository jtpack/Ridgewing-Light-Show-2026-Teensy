#include <Arduino.h>
#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <elapsedMillis.h>
#include <SparkFun_RV8803.h>


//
// Realtime Clock
//
RV8803 rtc;
int secondsElapsedSinceMidnight = 0;

//
// LEDS
//

// Configuration
#define COLOR_ORDER BRG

// LED arrays
#define LEFT_LED_PIN    1
#define LEFT_TOP_NUM_LEDS 28
#define LEFT_BOT_NUM_LEDS 55

#define RIGHT_LED_PIN   8
#define RIGHT_TOP_NUM_LEDS 14
#define RIGHT_BOT_NUM_LEDS 63

CRGBArray<LEFT_TOP_NUM_LEDS + LEFT_BOT_NUM_LEDS> leftLeds;
CRGBArray<RIGHT_TOP_NUM_LEDS + RIGHT_BOT_NUM_LEDS> rightLeds;

float tempo_bpm = 50.0;
float rightTopStartDelay = 0.015; // The proportion of the overall cycle that the right top leds wait before triggering
float leftBottomStartDelay = 0.15; // The proportion of the overall cycle that the left bottom leds wait before triggering
float rightBottomStartDelay = 0.15; // The proportion of the overall cycle that the right bottom leds wait before triggering

float leftTopDecay = 0.7; // The proportion of the overall cycle that the left top takes to decay to black
float leftBottomDecay = 0.7; // The proportion of the overall cycle that the left bottom takes to decay to black
float rightTopDecay = 0.7; // The proportion of the overall cycle that the right top takes to decay to black
float rightBottomDecay = 0.7; // The proportion of the overall cycle that the right bottom takes to decay to black

elapsedMillis heartbeatCycleStartTimer;
elapsedMillis leftTopStartTimer;
elapsedMillis leftBottomStartTimer;
elapsedMillis rightTopStartTimer;
elapsedMillis rightBottomStartTimer;

elapsedMillis leftTopDecayTimer;
elapsedMillis leftBottomDecayTimer;
elapsedMillis rightTopDecayTimer;
elapsedMillis rightBottomDecayTimer;

int heartbeatHue = 0;

CHSV leftTopColor = CHSV(heartbeatHue, 255, 0); // Start black
CHSV leftBottomColor = CHSV(heartbeatHue, 255, 0); // Start black
CHSV rightTopColor = CHSV(heartbeatHue, 255, 0); // Start black
CHSV rightBottomColor = CHSV(heartbeatHue, 255, 0); // Start black

CHSV leftTopPulseColor = CHSV(heartbeatHue, 255, 255);
CHSV leftBottomPulseColor = CHSV(heartbeatHue, 255, 255);
CHSV rightTopPulseColor = CHSV(heartbeatHue, 255, 255);
CHSV rightBottomPulseColor = CHSV(heartbeatHue, 255, 255);

int minBrightness = 0;
int maxBrightness = 255;

//
// Potentiometers
//

const int kMaxBrightnessPotPin = A12;
const int kMinBrightnessPotPin = A11;
const int kManualTempoPotPin = A10;

//
// Switches
//

const int kManualTempoSwitchPin = 32;

// When true, use the position of the manual tempo pot
// to set tempo rather than using the realtime clock.
bool manualTempoOverrideEnabled = false; 

//
// Status LEDs
//

const int kStatusLedPin = 2;

enum class StatusLedState {
  On,
  BlinkingSlow,
  BlinkingFast,
  Off
};

StatusLedState statusLedState = StatusLedState::Off;

const int kStatusLedSlowBlinkTime_ms = 1000;
const int kStatusLedFastBlinkTime_ms = 200;


void setup() {
  Serial.begin(9600);
  Serial.println("Starting Boot Process...");

  //
  // Prepare Status LED
  //
  pinMode(kStatusLedPin, OUTPUT);

  // Turn off status LED
  digitalWrite(kStatusLedPin, LOW);

  // Start with success LED status, and let
  // any error change it
  statusLedState = StatusLedState::On;

  //
  // Prepare LEDS
  //
  FastLED.addLeds<WS2812SERIAL, LEFT_LED_PIN, COLOR_ORDER>(leftLeds, leftLeds.size());
  FastLED.addLeds<WS2812SERIAL, RIGHT_LED_PIN, COLOR_ORDER>(rightLeds, rightLeds.size());
  FastLED.setBrightness(255);

  //
  // Prepare Potentiometers
  //
  pinMode(kMaxBrightnessPotPin, INPUT);
  pinMode(kMinBrightnessPotPin, INPUT);
  pinMode(kManualTempoPotPin, INPUT);
  
  //
  // Prepare Switch
  //
  pinMode(kManualTempoSwitchPin, INPUT_PULLUP);

  //
  // Prepare Realtime Clock
  //
  Wire.begin();
  if (rtc.begin() == false) {
    Serial.println("RTC not found. Using manual tempo control");
    statusLedState = StatusLedState::BlinkingFast;
  } else {
    Serial.println("Initialized RTC");
  }

  const bool needToSetTime = false;

  if (needToSetTime == true) {
    int year = 2025;
    int month = 4;
    int date = 3;
    int weekday = 5;
    int hour = 7; // Use 24 hour mode
    int minute = 43;
    int sec = 30;

    if (rtc.setTime(sec, minute, hour, weekday, date, month, year) == false) {
      Serial.println("Something went wrong setting the time");
    }
  }
  
  rtc.set24Hour();

  Serial.println("Booted.");
}



void loop() {
  static bool statusLedOn = false;
  static int blinkTime = 0;

  static bool waitingToStartLeftBottomPulse = false;
  static bool waitingToStartRightTopPulse = false;
  static bool waitingToStartRightBottomPulse = false;

  // 
  // Status LED
  //
  switch (statusLedState) {
    case StatusLedState::BlinkingFast:
      blinkTime = kStatusLedFastBlinkTime_ms;
      break;
    
    case StatusLedState::BlinkingSlow:
      blinkTime = kStatusLedSlowBlinkTime_ms;
      break;

    default:
      blinkTime = 0;
  }

  switch (statusLedState) {
    case StatusLedState::BlinkingSlow:
    case StatusLedState::BlinkingFast:
      EVERY_N_MILLIS(blinkTime) {
        statusLedOn = !statusLedOn;
      }
      break;

    case StatusLedState::Off:
      statusLedOn = false;
      break;

    case StatusLedState::On:
      statusLedOn = true;
      break;
  }

  digitalWrite(kStatusLedPin, HIGH ? statusLedOn == true : LOW);

  //
  // Realtime Clock
  //
  EVERY_N_SECONDS(1) {
    // Get the current time
    if (rtc.updateTime() == true) {
      String currentDate = rtc.stringDateUSA();
      String currentTime = rtc.stringTime();
      int hoursSinceMidnight = rtc.getHours();
      int minutesInThisHour = rtc.getMinutes();
      int secondsInThisHour = rtc.getSeconds();
      secondsElapsedSinceMidnight = hoursSinceMidnight * 3600 + minutesInThisHour * 60 + secondsInThisHour;
      Serial.print(currentDate);
      Serial.print(" ");
      Serial.print(currentTime);
      Serial.print(" - ");
      Serial.print(secondsElapsedSinceMidnight);
      Serial.println(" seconds elapsed since midnight");
    } else {
      Serial.println("Failed to read from RTC");
    }

  }

  //
  // Manual Override Switch
  // 
  static bool manualTempoSwitchValue = false;
  bool newManualTempoSwitchValue = digitalRead(kManualTempoSwitchPin) == LOW ? true : false;
  if (newManualTempoSwitchValue != manualTempoSwitchValue) {
    manualTempoSwitchValue = newManualTempoSwitchValue;

    if (manualTempoSwitchValue == true) {
      Serial.println("Manual tempo switch ON");
      manualTempoOverrideEnabled = true;
      statusLedState = StatusLedState::BlinkingSlow;
    } else {
      Serial.println("Manual tempo switch OFF");
      manualTempoOverrideEnabled = false;
      statusLedState = StatusLedState::On;
      // TODO: Gracefully handle RTC failure...
    }

  }

  if (manualTempoOverrideEnabled == true) {
    int newTempo = map(analogRead(kManualTempoPotPin), 0, 1023, 50, 28);
    if (newTempo != tempo_bpm) {
      Serial.print("Manual Tempo: ");
      Serial.println(newTempo);
      tempo_bpm = newTempo;
      // TODO: Do better debouncing of tempo control
    }

  } else {
    // TODO: Implement RTC-controlled tempo
    tempo_bpm = 60;
  }

  
  //
  // LED strips refresh
  //
  
  EVERY_N_MILLIS(16) {
    //
    // Read potentiometer positions
    //
    int newMaxBrightnessVal = map(analogRead(kMaxBrightnessPotPin), 0, 1023, 255, 0);
    if (newMaxBrightnessVal != maxBrightness) {
      maxBrightness = newMaxBrightnessVal;
      FastLED.setBrightness(maxBrightness);
      Serial.print("Max Brightness: ");
      Serial.println(maxBrightness);
    }

    int newMinBrightnessVal = map(analogRead(kMinBrightnessPotPin), 0, 1023, 255, 0);
    if (newMinBrightnessVal != minBrightness) {
      minBrightness = newMinBrightnessVal;
      Serial.print("Min Brightness: ");
      Serial.println(minBrightness);
    }

    unsigned int cycleDuration_ms = round((60.0 / tempo_bpm) * 1000.0); // How long between heartbeats
    //
    // Pulse Start Timers
    //
    unsigned int leftBottomStartDelay_ms = round(cycleDuration_ms * leftBottomStartDelay);
    unsigned int rightTopStartDelay_ms = round(cycleDuration_ms * rightTopStartDelay);
    unsigned int rightBottomStartDelay_ms = round(cycleDuration_ms * rightBottomStartDelay);

    if (heartbeatCycleStartTimer > cycleDuration_ms) {
      // Start the heartbeat.
      heartbeatCycleStartTimer = 0;

      // Start the left top pulse
      leftTopColor = leftTopPulseColor;

      // Start the left top decay timer
      leftTopDecayTimer = 0;
      
      // Start the timers for triggering the chambers
      waitingToStartLeftBottomPulse = true;
      leftBottomStartTimer = 0;
      
      waitingToStartRightTopPulse = true;
      rightTopStartTimer = 0;

      waitingToStartRightBottomPulse = true;
      rightBottomStartTimer = 0;
    }

    // Left bottom pulse timer
    //
    if (waitingToStartLeftBottomPulse == true && leftBottomStartTimer > leftBottomStartDelay_ms) {      
      // Start the pulse
      waitingToStartLeftBottomPulse = false;
      leftBottomColor = leftBottomPulseColor;

      // Start the decay timer
      leftBottomDecayTimer = 0;
    }

    // Right top pulse timer
    //
    if (waitingToStartRightTopPulse == true && rightTopStartTimer > rightTopStartDelay_ms) {
      // Start the pulse
      waitingToStartRightTopPulse = false;
      rightTopColor = rightTopPulseColor;

      // Start the decay timer
      rightTopDecayTimer = 0;
    }

    // Right bottom pulse timer
    //
    if (waitingToStartRightBottomPulse == true && heartbeatCycleStartTimer > rightBottomStartDelay_ms) {
      // Start the pulse
      waitingToStartRightBottomPulse = false;
      rightBottomColor = rightBottomPulseColor;
      
      // Start the decay timer
      rightBottomDecayTimer = 0;
    }
    
    //
    // LED refresh
    //

    // Refresh left top LEDs
    for (int i = LEFT_BOT_NUM_LEDS; i < LEFT_BOT_NUM_LEDS + LEFT_TOP_NUM_LEDS; i++) {
      leftLeds[i] = leftTopColor;
    }

    // Refresh left bottom LEDs
    for (int i = 0; i < LEFT_BOT_NUM_LEDS; i++) {
      leftLeds[i] = leftBottomColor;
    }

    // Refresh right top LEDs
    for (int i = RIGHT_BOT_NUM_LEDS; i < RIGHT_BOT_NUM_LEDS + RIGHT_TOP_NUM_LEDS; i++) {
      rightLeds[i] = rightTopColor;
    }

    // Refresh right bottom LEDs
    for (int i = 0; i < RIGHT_BOT_NUM_LEDS; i++) {
      rightLeds[i] = rightBottomColor;
    }

    FastLED.show();
    
    //
    // LED Decay
    //
      
    leftTopColor.v = max(leftTopPulseColor.v - (leftTopDecayTimer * leftTopPulseColor.v) / (cycleDuration_ms * leftTopDecay), minBrightness);
    leftBottomColor.v = max(leftBottomPulseColor.v - (leftBottomDecayTimer * leftBottomPulseColor.v) / (cycleDuration_ms * leftBottomDecay), minBrightness);
    rightTopColor.v = max(rightTopPulseColor.v - (rightTopDecayTimer * rightTopPulseColor.v) / (cycleDuration_ms * rightTopDecay), minBrightness);
    rightBottomColor.v = max(rightBottomPulseColor.v - (rightBottomDecayTimer * rightBottomPulseColor.v) / (cycleDuration_ms * rightBottomDecay), minBrightness);

    
    

  }

  // EVERY_N_SECONDS(2) {
  //   heartbeatHue += 1;
  //   if (heartbeatHue > 255) {
  //     heartbeatHue = 0;
  //   }
  //   leftBottomColor.h = heartbeatHue;
  //   leftBottomPulseColor.h = heartbeatHue;
  //   Serial.println(heartbeatHue);
  // }
    
    
}


// void printTime(){
//   Serial.print(rtc.getYear());
//   Serial.print("-");
//   Serial.print(rtc.getMonth());
//   Serial.print("-");
//   Serial.print(rtc.getDate());
//   Serial.print(" ");
//   Serial.print(rtc.getHour());
//   Serial.print(":");
//   Serial.print(rtc.getMinute());
//   Serial.print(":");
//   Serial.println(rtc.getSecond());
// }
