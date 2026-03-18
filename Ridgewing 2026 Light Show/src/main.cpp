#include <Arduino.h>
#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <elapsedMillis.h>
#include "Melopero_RV3028.h"

//
// Realtime Clock
//
Melopero_RV3028 rtc;


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

const int kMaxBrightnessPotPin = A0;
const int kMinBrightnessPotPin = A1;


// Forward declarations
void printTime();


void setup() {
    Serial.begin(9600);
    while (!Serial) delay(10);

    Serial.println("Boot");
    
    //
    // Prepare LEDS
    //
    FastLED.addLeds<WS2812SERIAL, LEFT_LED_PIN, COLOR_ORDER>(leftLeds, leftLeds.size());
    FastLED.addLeds<WS2812SERIAL, RIGHT_LED_PIN, COLOR_ORDER>(rightLeds, rightLeds.size());
    FastLED.setBrightness(255);

    // Prepare Potentiometers
    pinMode(kMaxBrightnessPotPin, INPUT);
    pinMode(kMinBrightnessPotPin, INPUT);

    // Prepare Realtime Clock
    Wire.begin();
    rtc.initI2C();

    // Use 24-hour mode
    rtc.set24HourMode();

    //rtc.setTime(2026, 3, 4, 18, 7, 14, 0);

    printTime();


}



void loop() {
  static bool waitingToStartLeftBottomPulse = false;
  static bool waitingToStartRightTopPulse = false;
  static bool waitingToStartRightBottomPulse = false;

  //
  // Realtime Clock
  //
  EVERY_N_SECONDS(1) {
    // Get the current time
    printTime();

    // Calculate how far we are into the day
    int secondsElapsedToday = (rtc.getHour() * 3600) + (rtc.getMinute() * 60) + rtc.getSecond();
    Serial.println(secondsElapsedToday);
  }
  
  //
  // LED strips refresh
  //
  
  EVERY_N_MILLIS(16) {
    //
    // Read potentiometer positions
    //
    int newMaxBrightnessVal = map(analogRead(kMaxBrightnessPotPin), 0, 1023, 0, 255);
    if (newMaxBrightnessVal != maxBrightness) {
      maxBrightness = newMaxBrightnessVal;
      FastLED.setBrightness(maxBrightness);
      Serial.print("Max Brightness: ");
      Serial.println(maxBrightness);
    }

    int newTempo = map(analogRead(kMinBrightnessPotPin), 0, 1023, 28, 50);
    if (newTempo != tempo_bpm) {
      Serial.print("New Tempo: ");
      Serial.println(newTempo);
      tempo_bpm = newTempo;
    }

    // int newMinBrightnessVal = map(analogRead(kMinBrightnessPotPin), 0, 1023, 0, 255);
    // if (newMinBrightnessVal != minBrightness) {
    //   minBrightness = newMinBrightnessVal;
    //   Serial.print("Min Brightness: ");
    //   Serial.println(minBrightness);
    // }

    // int newHue = map(analogRead(kMinBrightnessPotPin), 0, 1023, 0, 255);
    // if (newHue != heartbeatHue) {
    //   Serial.print("Hue: ");
    //   Serial.println(newHue);
    //   heartbeatHue = newHue;
    //   leftTopColor.h = heartbeatHue;
    //   leftBottomColor.h = heartbeatHue;
    //   rightTopColor.h = heartbeatHue;
    //   rightBottomColor.h = heartbeatHue;
    // }

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
    
    
}


void printTime(){
  Serial.print(rtc.getYear());
  Serial.print("-");
  Serial.print(rtc.getMonth());
  Serial.print("-");
  Serial.print(rtc.getDate());
  Serial.print(" ");
  Serial.print(rtc.getHour());
  Serial.print(":");
  Serial.print(rtc.getMinute());
  Serial.print(":");
  Serial.println(rtc.getSecond());
}
