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
 * @file telemetry.h
 * @brief Header file for the telemetry module.
 *
 * This module handles the initialization and periodic updates of the Inertial Measurement Unit (IMU)
 * and the broadcasting of sensor data. It uses the ArduinoJson library for creating JSON payloads
 * to send over a network connection.
 */
 
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <RobOra_42670.h>
#include "config.h"
#include "websocket.h"

/// @brief Variable to store the latest IMU data frame.
extern _sRobOra_42670_IMU imuFrame;

/**
 * @brief Initializes the IMU and telemetry systems.
 *
 * This function sets up the I2C bus and initializes the IMU sensor.
 */
void telemetryInit();

/**
 * @brief Main function to be called cyclically in the loop() function.
 *
 * This function updates the IMU data and periodically broadcasts sensor telemetry.
 */
void telemetryTick();

/**
 * @brief Re-Initializes the IMU and telemetry systems
 *
 * This function sets up the I2C bus and re-initializes the IMU sensor
 */
void telemetryReload();

/**
 * @brief Reads a raw analog value from the specified pin, converts it to a
 * scaled voltage, and then calculates the actual voltage based on the
 * external voltage divider ratio.
 * * This function handles the full conversion process:
 * 1. Reads the 12-bit raw value (0-4095) from the ADC.
 * 2. Scales the raw value to the voltage at the ADC pin (V_out), 
 * using the predefined constants (MAX_ADC_VOLTAGE and ADC_RESOLUTION).
 * 3. Applies the external VOLTAGE_DIVIDER_RATIO to find the original 
 * input voltage (V_in), which is the final telemetry reading.
 *
 * @param Pin The GPIO pin number (uint8_t) configured as an analog input 
 * @return float The actual voltage measured at the source (V_in), in Volts.
 */
float telemetryReadAdC(uint8_t Pin);