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
 * @file utility.h
 * @brief Header file for the utility module.
 *
 * This module handles the initialization and periodic updates of the Inertial Measurement Unit (IMU)
 * and the broadcasting of sensor data. It uses the ArduinoJson library for creating JSON payloads
 * to send over a network connection.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Schedules a deferred device reboot.
 * @param ms The number of milliseconds to wait before rebooting.
 * @note Creates a low-priority task constrained to core 0.
 */
void scheduleReboot(unsigned long ms);

/**
 * @brief Check if I2C address is connect to the bus
 * @param address The address of device
 * @return false if there are no device, true otherwise.
 */
bool checkI2CDevice(byte address);

/**
 * @brief Concatenates two strings with left padding.
 *
 * @param base The starting string (prefix).
 * @param text The string to append.
 * @param totalWidth The desired total width of the resulting string.
 * @param padChar The padding character (default: space).
 * @return A new String with `text` right-aligned with respect to the padding and concatenated to `base`.
 *
 * @example
 * String s = padLeft("SSID:", "PIPPO", 10);
 * // Result: "SSID: PIPPO"
 */
String padLeft(const String &base, const String &text, int totalWidth, char padChar = ' ');

/**
 * @brief Concatenates two strings with padding on the right.
 *
 * @param base The starting string (prefix).
 * @param text The string to append.
 * @param totalWidth The desired total width of the resulting string.
 * @param padChar The padding character (default: space).
 * @return A new String with `text` left-aligned with respect to the padding and concatenated to `base`.
 *
 * @example
 * String s = padRight("SSID:", "PIPPO", 10);
 * // Result: "SSID:PIPPO"
 */
String padRight(const String &base, const String &text, int totalWidth, char padChar = ' ');

/**
 * @brief Centers a string within a given total width (left and right padding).
 *
 * @param text Text to center.
 * @param totalWidth Desired total width.
 * @param padChar Pad character (default: space).
 * @return Centered string (if totalWidth <= len, returns text).
 *
 * @example
 * padCenter("OK", 6) -> " OK "
 */
String padCenter(const String &text, int totalWidth, char padChar = ' ');