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
 * @file utility.cpp
 * @brief Implementation of the telemetry module.
 *
 * This file contains the utility.
 */
#include "utility.h"

/**
 * @brief Creates a task that performs a reboot after @p ms milliseconds.
 * @param ms The number of milliseconds to wait.
 * @note Uses xTaskCreatePinnedToCore with priority 1 on core 0.
 */
void scheduleReboot(unsigned long ms)
{
  xTaskCreatePinnedToCore([](void *p)
                          {
        vTaskDelay(pdMS_TO_TICKS((uint32_t)p));
        ESP.restart(); }, "reboot", 2048, (void *)ms, 1, nullptr, 0);
}

/**
 * @brief Check if I2C address is connect to the bus
 * @param address The address of device
 * @return false if there are no device, true otherwise.
 */
bool checkI2CDevice(byte address)
{
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0) ? true : false;
}

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
String padLeft(const String &base, const String &text, int totalWidth, char padChar)
{
  int padCount = (totalWidth - base.length()) - text.length();
  if (padCount <= 0)
    return text;

  String result = base;
  result.reserve(totalWidth); // evita riallocazioni dinamiche multiple
  for (int i = 0; i < padCount; i++)
    result += padChar;
  result += text;
  return result;
}

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
String padRight(const String &base, const String &text, int totalWidth, char padChar)
{
  int padCount = (totalWidth - base.length()) - text.length();

  if (padCount <= 0)
    return text;

  String result = text;
  result.reserve(totalWidth);
  for (int i = 0; i < padCount; i++)
    result += padChar;
  return result;
}

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
String padCenter(const String &text, int totalWidth, char padChar)
{
  int pad = totalWidth - text.length();
  if (pad <= 0)
    return text;

  int i;
  int left = pad / 2;
  int right = pad - left;

  String out;
  out.reserve(totalWidth);
  for (i = 0; i < left; i++)
    out += padChar;
  out += text;
  for (i = 0; i < left; i++)
    out += padChar;
  return out;
}