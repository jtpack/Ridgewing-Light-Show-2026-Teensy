#include <Arduino.h>
#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <elapsedMillis.h>


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


float tempo_bpm = 20.0;
float bottomDelay = 0.2; // The proportion of the overall cycle that the bottom leds wait before triggering
float rightSideDelay = 0.1; // The proportion of the overall cycle that the right side waits before following the left side

unsigned int cycleDuration_ms = round((60.0 / tempo_bpm) * 1000.0); // How long between heartbeats
unsigned int pulse2Delay_ms = round(cycleDuration_ms * bottomDelay); // Pulse 2 follows pulse 1 after this many ms.
unsigned int rightSideDelay_ms = round(cycleDuration_ms * rightSideDelay);

elapsedMillis leftTopPulseTimer;
elapsedMillis leftBottomPulseTimer;
elapsedMillis rightPulse1Timer;
elapsedMillis rightPulse2Timer;

int leftTopPulseDecay = round(tempo_bpm * 0.3);
int leftBottomPulseDecay = round(tempo_bpm * 0.2);
int rightTopPulseDecay = round(tempo_bpm * 0.33);
int rightBottomPulseDecay = round(tempo_bpm * 0.24);

CHSV leftTopColor = CHSV(0, 255, 0); // Start black
CHSV leftBottomColor = CHSV(0, 255, 0); // Start black
CHSV rightTopColor = CHSV(0, 255, 0); // Start black
CHSV rightBottomColor = CHSV(0, 255, 0); // Start black

CHSV leftTopPulseColor = CHSV(0, 255, 200);
CHSV leftBottomPulseColor = CHSV(0, 255, 255);
CHSV rightTopPulseColor = CHSV(0, 255, 200);
CHSV rightBottomPulseColor = CHSV(0, 255, 255);



void setup() {
    // Initialize FastLED
    FastLED.addLeds<WS2812SERIAL, LEFT_LED_PIN, COLOR_ORDER>(leftLeds, leftLeds.size());
    FastLED.addLeds<WS2812SERIAL, RIGHT_LED_PIN, COLOR_ORDER>(rightLeds, rightLeds.size());
    
    FastLED.setBrightness(255);
    Serial.begin(9600);
    Serial.println("Boot");
}



void loop() {
  static bool inPulse1 = false;
  static bool inPulse2 = false;



  // if (leftTopPulseTimer > cycleDuration_ms) {
  //   // Start the 1st pulse of the heartbeat
  //   inPulse1 = true;
  //   leftTopPulseTimer = 0;
  //   leftCurrColor = leftTopPulseColor;
    
  //   // Reset the timer for the trigger of pulse 2
  //   leftBottomPulseTimer = 0;

  //   // We are no longer in pulse 2
  //   inPulse2 = false;
  // }

  // if (inPulse1 == true && leftBottomPulseTimer > pulse2Delay_ms) {
  //   // Start the 2nd pulse
  //   inPulse2 = true;
  //   leftCurrColor = leftBottomPulseColor;

  //   // We are no longer in pulse 1
  //   inPulse1 = false;
  // }
  


  EVERY_N_MILLIS(16) {
    leftTopColor = CHSV(0, 255, 255);
    leftBottomColor = CHSV(50, 255, 255);
    rightTopColor = CHSV(100, 255, 255);
    rightBottomColor = CHSV(150, 255, 255);
    
    // Update Left Top LEDs
    for (int i = LEFT_BOT_NUM_LEDS; i < LEFT_BOT_NUM_LEDS + LEFT_TOP_NUM_LEDS; i++) {
      leftLeds[i] = leftTopColor;
    }

    // Update Left Bottom LEDs
    for (int i = 0; i < LEFT_BOT_NUM_LEDS; i++) {
      leftLeds[i] = leftBottomColor;
    }

    // Update Left Top LEDs
    for (int i = RIGHT_BOT_NUM_LEDS; i < RIGHT_BOT_NUM_LEDS + RIGHT_TOP_NUM_LEDS; i++) {
      rightLeds[i] = rightTopColor;
    }

    // Update Left Bottom LEDs
    for (int i = 0; i < RIGHT_BOT_NUM_LEDS; i++) {
      rightLeds[i] = rightBottomColor;
    }

    FastLED.show();
    
    // // Apply fadeout
    // if (inPulse1 == true) {
    //   leftCurrColor.v = max(leftCurrColor.v - leftTopPulseDecay, 0);
    // } else if (inPulse2 == true) {
    //   leftCurrColor.v = max(leftCurrColor.v - leftBottomPulseDecay, 0);
    // }

  }
    
    
}


void noiseEffect() {
  CRGBPalette16 palette1 = LavaColors_p;
  CRGBPalette16 palette2 = CloudColors_p;
  static uint16_t z = 0;

    for (int i = 0; i < NUM_LEDS_1; i++) {
        // Generate noise value (0-255)
        uint8_t noise = inoise8(
          i * 10, 
          millis() / 25 + i * 15
        );

        // Map to color (hue)
        leds1[i] = ColorFromPalette(palette1, noise, 255, LINEARBLEND);
    }

    for (int i = 0; i < NUM_LEDS_2; i++) {
        uint8_t noise = inoise8(
          i * 10, 
          millis() / 30
        );

        // Map to color (hue)
        leds2[i] = ColorFromPalette(palette2, noise, 255, LINEARBLEND);
    }

    z += 1;

    

}


// void fadeInOut() {
//     static uint8_t brightness = 0;
//     static int8_t direction = 1;

//     EVERY_N_MILLISECONDS(10) {
//         brightness += direction;

//         if (brightness == 0 || brightness == 255) {
//             direction = -direction;
//         }

//         FastLED.setBrightness(brightness);
//         fill_solid(leds, NUM_LEDS_1, CRGB::Purple);
//         FastLED.show();
//     }
// }







// /*  OctoWS2811 BasicTest.ino - Basic RGB LED Test
//     http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
//     Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC

//     Permission is hereby granted, free of charge, to any person obtaining a copy
//     of this software and associated documentation files (the "Software"), to deal
//     in the Software without restriction, including without limitation the rights
//     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//     copies of the Software, and to permit persons to whom the Software is
//     furnished to do so, subject to the following conditions:

//     The above copyright notice and this permission notice shall be included in
//     all copies or substantial portions of the Software.

//     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//     THE SOFTWARE.

//   Required Connections
//   --------------------
//     pin 2:  LED Strip #1    OctoWS2811 drives 8 LED Strips.
//     pin 14: LED strip #2    All 8 are the same length.
//     pin 7:  LED strip #3
//     pin 8:  LED strip #4    A 100 ohm resistor should used
//     pin 6:  LED strip #5    between each Teensy pin and the
//     pin 20: LED strip #6    wire to the LED strip, to minimize
//     pin 21: LED strip #7    high frequency ringining & noise.
//     pin 5:  LED strip #8
//     pin 15 & 16 - Connect together, but do not use
//     pin 4 - Do not use
//     pin 3 - Do not use as PWM.  Normal use is ok.

//   This test is useful for checking if your LED strips work, and which
//   color config (WS2811_RGB, WS2811_GRB, etc) they require.
// */

// #include <OctoWS2811.h>

// // These buffers need to be large enough for all the pixels.
// // The total number of pixels is "ledsPerStrip * numLedStrips".
// // Each pixel needs 3 bytes, so multiply by 3.  An "int" is
// // 4 bytes, so divide by 4.  The array is created using "int"
// // so the compiler will align it to 32 bit memory.

// const int numLedStrips = 1;
// byte pinList[numLedStrips] = {2};
// const int ledsPerStrip = 288;
// const int bytesPerLED = 3; // RGB

// DMAMEM int displayMemory[ledsPerStrip * numLedStrips * bytesPerLED / 4];
// int drawingMemory[ledsPerStrip * numLedStrips * bytesPerLED / 4];

// const int config = WS2811_GRB | WS2811_800kHz;

// OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config, numLedStrips, pinList);

// void colorWipe(int color, int wait);

// void setup() {
//   leds.begin();
//   leds.show();
// }

// #define RED    0xFF0000
// #define GREEN  0x00FF00
// #define BLUE   0x0000FF
// #define YELLOW 0xFFFF00
// #define PINK   0xFF1088
// #define ORANGE 0xE05800
// #define WHITE  0xFFFFFF

// // Less intense...
// /*
// #define RED    0x160000
// #define GREEN  0x001600
// #define BLUE   0x000016
// #define YELLOW 0x101400
// #define PINK   0x120009
// #define ORANGE 0x100400
// #define WHITE  0x101010
// */

// void loop() {
//   int microsec = 2000000 / leds.numPixels();  // change them all in 2 seconds

//   // uncomment for voltage controlled speed
//   // millisec = analogRead(A9) / 40;

//   colorWipe(RED, microsec);
//   colorWipe(GREEN, microsec);
//   colorWipe(BLUE, microsec);
//   colorWipe(YELLOW, microsec);
//   colorWipe(PINK, microsec);
//   colorWipe(ORANGE, microsec);
//   colorWipe(WHITE, microsec);
// }

// void colorWipe(int color, int wait)
// {
//   for (int i=0; i < leds.numPixels(); i++) {
//     leds.setPixel(i, color);
//     leds.show();
//     delayMicroseconds(wait);
//   }
// }