/*
 * Copyright (c) 2025 Orazio Franco <robora2025@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/**
 * @file ledsrgb.cpp
 * @brief This file contains the implementation for controlling a single RGB LED.
 * * This file contains the declarations for functions that initialize and control
 * a single RGB LED, likely a NeoPixel or similar addressable LED. It provides
 * functions to set the LED to a specific RGB color, as well as convenience
 * functions for common colors like red, green, blue, on, and off.
 */
#include "ledsrgb.h"

/**
 * @brief NeoPixel object that represents the LED strip.
 */
Adafruit_NeoPixel *strip;
/**
 * @brief Number of LED strip.
 */
static uint8_t nLed;

/**
 * @brief LED rgb tick.
 */
static uint32_t ledsTick = 0;

/**
 * @brief Initializes the LED strip and its settings.
 * * @details This function starts the NeoPixel strip, sets its initial brightness,
 * and performs a clear operation by calling `strip.show()`.
 * * @param n The number of leds.
 * * @param pin The pin still connect leds
 * * @param brightness The initial brightness level (0-255).
 */
void ledsInit(uint16_t n, int16_t pin, uint8_t brightness)
{

  if (strip == nullptr)
    strip = new Adafruit_NeoPixel(n, pin, NEO_GRB + NEO_KHZ800);

  nLed = n;
  strip->begin();
  strip->setBrightness(brightness);
  strip->show(); // Clear
}

/**
 * @brief Sets the color of the first pixel and updates the strip.
 * * @details This function sets the color of the single pixel at index 0 and then
 * calls `strip.show()` to push the new color to the physical LED.
 * * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 */
void ledsSetRGB(uint8_t r, uint8_t g, uint8_t b)
{
  strip->setPixelColor((nLed - 1), strip->Color(r, g, b));
  strip->show();
}

 
/**
 * @brief Sets the color rainbow.
 * * @details This function sets the rainbow color of all the pixel at index 0
 */
void ledsSetRAINBOW(void)
{
  static uint32_t hue = 0;
  uint32_t now = millis();
  if ((now - ledsTick) >= 10)
  {
    ledsTick = now;
    strip->setPixelColor((nLed - 1), strip->gamma32(strip->ColorHSV(hue, 255, 255)));
    hue += 100;
    if (hue > 65536)
      hue = 0;
  }
  strip->show();
}
