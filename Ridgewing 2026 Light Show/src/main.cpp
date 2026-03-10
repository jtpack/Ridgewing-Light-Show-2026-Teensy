#include <Arduino.h>


#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>


// Configuration
#define LED_PIN_1     1
#define NUM_LEDS_1    144
#define LED_PIN_2     8
#define NUM_LEDS_2    144
#define COLOR_ORDER BRG

// LED array
CRGBArray<NUM_LEDS_1> leds1;
CRGBArray<NUM_LEDS_2> leds2;

void fadeInOut();
void noiseEffect();

void setup() {
    // Initialize FastLED
    FastLED.addLeds<WS2812SERIAL, LED_PIN_1, COLOR_ORDER>(leds1, leds1.size());
    FastLED.addLeds<WS2812SERIAL, LED_PIN_2, COLOR_ORDER>(leds2, leds2.size());
    FastLED.setBrightness(50);
    
}



void loop() {
  EVERY_N_MILLIS(17) {
    // leds2.fill_solid(CRGB::Blue);
    noiseEffect();
    FastLED.show();
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