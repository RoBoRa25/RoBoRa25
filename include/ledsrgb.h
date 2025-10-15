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
 * @file ledsrgb.h
 * @brief Header file for controlling a single RGB LED.
 * * This file contains the declarations for functions that initialize and control 
 * a single RGB LED, likely a NeoPixel or similar addressable LED. It provides
 * functions to set the LED to a specific RGB color, as well as convenience
 * functions for common colors like red, green, blue, on, and off.
 */
 
#pragma once
#include <stdint.h>
#include <Adafruit_NeoPixel.h>

/**
 * @brief Initializes the LED strip and its settings.
 * * @details This function starts the NeoPixel strip, sets its initial brightness,
 * and performs a clear operation by calling `strip.show()`.
 * * @param n The number of leds.
 * * @param pin The pin still connect leds
 * * @param brightness The initial brightness level (0-255).
 */
void ledsInit(uint16_t n, int16_t pin, uint8_t brightness = 50);

/**
 * @brief Sets the LED to a specific RGB color and displays it.
 * * This is the main function for controlling the LED's color. It takes three
 * 8-bit values for the red, green, and blue components.
 * * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 */
void ledsSetRGB(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Turns the LED off.
 * * @details This is an inline convenience function that calls `ledsSetRGB(0,0,0)`.
 */
inline void ledsOff(){ ledsSetRGB(0,0,0); }

/**
 * @brief Turns the LED on to full white brightness.
 * * @details This is an inline convenience function that calls `ledsSetRGB(255,255,255)`.
 */
 
inline void ledsON(){ ledsSetRGB(255,255,255); }

/**
 * @brief Sets the LED color to full red.
 * * @details This is an inline convenience function that calls `ledsSetRGB(255, 0, 0)`.
 */
inline void ledsR(){ ledsSetRGB(255, 0, 0); }

/**
 * @brief Sets the LED color to full green.
 * * @details This is an inline convenience function that calls `ledsSetRGB(0, 255, 0)`.
 */
inline void ledsG(){ ledsSetRGB(0, 255, 0); }

/**
 * @brief Sets the LED color to full blue.
 * * @details This is an inline convenience function that calls `ledsSetRGB(0, 0, 255)`.
 */
inline void ledsB(){ ledsSetRGB(0, 0, 255); }

/**
 * @brief Sets the color rainbow.
 * * @details This function sets the rainbow color of all the pixel at index 0 
 */
void ledsSetRAINBOW(void);
