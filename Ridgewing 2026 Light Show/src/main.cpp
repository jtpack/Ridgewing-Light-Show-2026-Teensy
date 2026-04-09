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
bool rtcSuccessfullyInitialized = false;
int rtcFailureFallbackTempo = 40;

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

float tempo_bpm = 40.0;

// Proportion of overall cycle that each section of LEDs waits to trigger
float rightTopStartDelay = 0.015;
float leftBottomStartDelay = 0.15;
float rightBottomStartDelay = 0.15;

// Proportion of overall cycle required to fade down to black
float leftTopDecay = 0.7; 
float leftBottomDecay = 0.7; 
float rightTopDecay = 0.7;
float rightBottomDecay = 0.7;

float leftTopBrightness = 0.0;
float leftBottomBrightness = 0.0;
float rightTopBrightness = 0.0;
float rightBottomBrightness = 0.0;

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

//
// Potentiometers
//

const int kMaxBrightnessPotPin = A10; // A
const int kMinBrightnessPotPin = A11; // B
const int kManualTempoPotPin = A12; // C

int kMaxBrightnessControlMinValue = 40;
int kMinBrightnessControlMaxValue = kMaxBrightnessControlMinValue;
int kManualTempoControlMinValue = 24;
int kManualTempoControlMaxValue = 45;

const int kNumPotReadsToAverage = 10;

int minBrightness = 0;
int maxBrightness = 255;

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


//
// Gamma Correction
//

float gammaCorrectionFactor = 1.5;

//
// Forward Declarations
//

int gammaCorrectedValue(int value, float correctionFactor);
float gammaCorrectedFloatValue(float value, float correctionFactor);


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
    Serial.println("RTC not found");
    statusLedState = StatusLedState::BlinkingFast;
    rtcSuccessfullyInitialized = false;
  } else {
    Serial.println("Initialized RTC");
    rtcSuccessfullyInitialized = true;
  }

  const bool needToSetTime = false;

  if (needToSetTime == true) {
    int year = 2026;
    int month = 4;
    int date = 8;
    int weekday = 5; // Sunday is 1
    int hour = 20; // Use 24 hour mode
    int minute = 33;
    int sec = 0;

    if (rtc.setTime(sec, minute, hour, weekday, date, month, year) == false) {
      Serial.println("Something went wrong setting the time");
      statusLedState = StatusLedState::BlinkingFast;
      rtcSuccessfullyInitialized = false;
    }
  }
  
  rtc.set24Hour();

  // Get initial control positions
  manualTempoOverrideEnabled = digitalRead(kManualTempoSwitchPin) == LOW ? true : false;
  if (manualTempoOverrideEnabled == true) {
    tempo_bpm = map(analogRead(kManualTempoPotPin), 0, 1023, kManualTempoControlMinValue, kManualTempoControlMaxValue);
  }

  maxBrightness = map(analogRead(kMaxBrightnessPotPin), 0, 1023, kMaxBrightnessControlMinValue, 255);
  minBrightness = map(analogRead(kMinBrightnessPotPin), 0, 1023, 0, kMinBrightnessControlMaxValue);

  if (manualTempoOverrideEnabled == false) {
      if (rtcSuccessfullyInitialized == true) {
        Serial.println("Using RTC for automatic tempo control");
      } else {
        tempo_bpm = rtcFailureFallbackTempo;
        Serial.print("Using fallback tempo of ");
        Serial.print(rtcFailureFallbackTempo);
        Serial.println(" BPM");
      }
    } else {
      Serial.print("Manual tempo control active. Tempo: ");
      Serial.print(tempo_bpm);
      Serial.println(" BPM");
    }

    Serial.print("Max Brightness: ");
    Serial.println(maxBrightness);

    Serial.print("Min Brightness: ");
    Serial.println(minBrightness);
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
    if (manualTempoOverrideEnabled == false) {
      if (rtcSuccessfullyInitialized == true) {
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

          // TODO: Implement RTC-controlled tempo
          tempo_bpm = 60;
        } else {
          Serial.println("Failed to read from RTC.");
          rtcSuccessfullyInitialized = false;
          statusLedState = StatusLedState::BlinkingFast;
          tempo_bpm = rtcFailureFallbackTempo;
          Serial.print("Setting tempo to fallback ");
          Serial.print(tempo_bpm);
          Serial.println(" BPM");
        }
      }
    }
  }

  EVERY_N_MILLIS(10) {
    //
    // Read potentiometer positions
    //
    int potValAvg = 0;

    // Max Brightness Control
    //
    for (int i = 0; i < kNumPotReadsToAverage; i++) {
      potValAvg += analogRead(kMaxBrightnessPotPin);
    }
    potValAvg = (int) round((float) potValAvg / (float) kNumPotReadsToAverage);
    if (potValAvg >= 1006) potValAvg = 1023;
    if (potValAvg <= 20) potValAvg = 0;

    int newMaxBrightnessVal = map(potValAvg, 0, 1023, kMaxBrightnessControlMinValue, 255);
    
    if (abs(newMaxBrightnessVal - maxBrightness) > 1) {
      maxBrightness = newMaxBrightnessVal;
      FastLED.setBrightness(maxBrightness);
      Serial.print("Max Brightness: ");
      Serial.println(maxBrightness);
    }

    // Min Brightness Control
    //
    potValAvg = 0;
    for (int i = 0; i < kNumPotReadsToAverage; i++) {
      potValAvg += analogRead(kMinBrightnessPotPin);
    }
    potValAvg = (int) round((float) potValAvg / (float) kNumPotReadsToAverage);
    if (potValAvg >= 1006) potValAvg = 1023;
    if (potValAvg <= 20) potValAvg = 0;

    int newMinBrightnessVal = map(potValAvg, 0, 1023, 0, kMinBrightnessControlMaxValue);
    if (abs(newMinBrightnessVal - minBrightness) > 1) {
      minBrightness = newMinBrightnessVal;
      Serial.print("Min Brightness: ");
      Serial.println(minBrightness);
    }

    // Manual Tempo Control
    potValAvg = 0;
    for (int i = 0; i < kNumPotReadsToAverage; i++) {
      potValAvg += analogRead(kManualTempoPotPin);
    }
    potValAvg = (int) round((float) potValAvg / (float) kNumPotReadsToAverage);
    if (potValAvg >= 1006) potValAvg = 1023;
    if (potValAvg <= 20) potValAvg = 0;

    if (manualTempoOverrideEnabled == true) {
      int newTempo = map(potValAvg, 0, 1023, kManualTempoControlMinValue, kManualTempoControlMaxValue);
      if (abs(newTempo - tempo_bpm) > 1) {
        Serial.print("Manual Tempo: ");
        Serial.println(newTempo);
        tempo_bpm = newTempo;
      }
    }

    //
    // Manual Tempo Switch
    // 
    bool newManualTempoSwitchValue = digitalRead(kManualTempoSwitchPin) == LOW ? true : false;
    if (newManualTempoSwitchValue != manualTempoOverrideEnabled) {
      manualTempoOverrideEnabled = newManualTempoSwitchValue;

      if (manualTempoOverrideEnabled == true) {
        Serial.println("Manual tempo switch ON");
      } else {
        Serial.println("Manual tempo switch OFF");
      }
    }
  }

  
  //
  // LED strips refresh
  //
  
  EVERY_N_MILLIS(16) {
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
      leftTopBrightness = 1.0;

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
      leftBottomBrightness = 1.0;

      // Start the decay timer
      leftBottomDecayTimer = 0;
    }

    // Right top pulse timer
    //
    if (waitingToStartRightTopPulse == true && rightTopStartTimer > rightTopStartDelay_ms) {
      // Start the pulse
      waitingToStartRightTopPulse = false;
      rightTopBrightness = 1.0;

      // Start the decay timer
      rightTopDecayTimer = 0;
    }

    // Right bottom pulse timer
    //
    if (waitingToStartRightBottomPulse == true && heartbeatCycleStartTimer > rightBottomStartDelay_ms) {
      // Start the pulse
      waitingToStartRightBottomPulse = false;
      rightBottomBrightness = 1.0;
      
      // Start the decay timer
      rightBottomDecayTimer = 0;
    }
    
    //
    // LED refresh
    //

    float heartbeatProgress = (float) heartbeatCycleStartTimer / (float) cycleDuration_ms;
    

    // Left Top
    //
    float leftTopProgress = heartbeatProgress / leftTopDecay;
    if (leftTopProgress < 1.0) {
      leftTopBrightness = 1.0 - leftTopProgress;
    } else {
      leftTopBrightness = 0.0;
    }

    int leftTopLedValue = int(round(leftTopBrightness * 255.0));
    leftTopLedValue = max(leftTopLedValue, minBrightness);

    CHSV leftTopColor = CHSV(heartbeatHue, 255, leftTopLedValue);

    // Refresh left top LEDs
    for (int i = LEFT_BOT_NUM_LEDS; i < LEFT_BOT_NUM_LEDS + LEFT_TOP_NUM_LEDS; i++) {
      leftLeds[i] = leftTopColor;
    }


    // Left Bottom
    //
    float leftBottomProgress = (heartbeatProgress - leftBottomStartDelay) / leftBottomDecay;
    if (leftBottomProgress >= 0.0 && leftBottomProgress < 1.0) {
      leftBottomBrightness = 1.0 - leftBottomProgress;
    } else {
      leftBottomBrightness = 0.0;
    }

    int leftBottomLedValue = int(round(leftBottomBrightness * 255.0));
    leftBottomLedValue = max(leftBottomLedValue, minBrightness);

    CHSV leftBottomColor = CHSV(heartbeatHue, 255, leftBottomLedValue);

    // Refresh left bottom LEDs
    for (int i = 0; i < LEFT_BOT_NUM_LEDS; i++) {
      leftLeds[i] = leftBottomColor;
    }


    // Right Top
    //
    float rightTopProgress = (heartbeatProgress - rightTopStartDelay) / rightTopDecay;
    if (rightTopProgress >= 0.0 && rightTopProgress < 1.0) {
      rightTopBrightness = 1.0 - rightTopProgress;
    } else {
      rightTopBrightness = 0.0;
    }

    int rightTopLedValue = int(round(rightTopBrightness * 255.0));
    rightTopLedValue = max(rightTopLedValue, minBrightness);

    CHSV rightTopColor = CHSV(heartbeatHue, 255, rightTopLedValue);

    // Refresh right bottom LEDs
    for (int i = RIGHT_BOT_NUM_LEDS; i < RIGHT_BOT_NUM_LEDS + RIGHT_TOP_NUM_LEDS; i++) {
      rightLeds[i] = rightTopColor;
    }


    // Right Bottom
    //
    float rightBottomProgress = (heartbeatProgress - rightBottomStartDelay) / rightBottomDecay;
    if (rightBottomProgress >= 0.0 && rightBottomProgress < 1.0) {
      rightBottomBrightness = 1.0 - rightBottomProgress;
    } else {
      rightBottomBrightness = 0.0;
    }

    int rightBottomLedValue = int(round(rightBottomBrightness * 255.0));
    rightBottomLedValue = max(rightBottomLedValue, minBrightness);

    CHSV rightBottomColor = CHSV(heartbeatHue, 255, rightBottomLedValue);

    // Refresh right bottom LEDs
    for (int i = 0; i < RIGHT_BOT_NUM_LEDS; i++) {
      rightLeds[i] = rightBottomColor;
    }


    FastLED.show();
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

int gammaCorrectedValue(int value, float correctionFactor) {
  // Return a gamma-corrected LED intensity value
  float correctedVal = pow((float) value / (float) 255, correctionFactor) * 255 + 0.5;
  return round(correctedVal);
}

float gammaCorrectedFloatValue(float value, float correctionFactor) {
  // Return a gamma-corrected float LED intensity value.
  // Input value must be in the range 0.0 to 1.0
  return pow(value, correctionFactor);
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
