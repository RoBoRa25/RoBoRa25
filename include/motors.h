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
 * @file motors.h
 * @brief This is the header file for the motor control module.
 *
 * It defines the public function prototypes for initializing, controlling,
 * and getting information about the motors.
 */
#pragma once
#include <Arduino.h>
#include <RoBoRa_8833.h>
#include "config.h"

/**
 * @brief Initializes the motor control system.
 *
 * This function reads the motor configuration and sets up the hardware.
 * @see motorsInit() in motors.cpp for implementation details.
 */
void motorsInit();

/**
 * @brief Re-Initializesthe motor control system
 *
 * This function reads the motor configuration and sets up the hardware.
 */
void motorsReload();

/**
 * @brief Handles periodic motor updates.
 *
 * This function should be called frequently within the main loop to ensure
 * motor commands are re-applied periodically.
 * @see motorsTick() in motors.cpp for implementation details.
 */
void motorsTick();

/**
 * @brief Applies new throttle and steering commands.
 *
 * @param throttle The throttle value.
 * @param steer The steering value.
 */
void motorsApply(int16_t throttle, int16_t steer);

/**
 * @brief Prints the current motor configuration to the serial port.
 */
void motorsPrintConfig();

/**
 * @brief Get Last value of throttle
 *
 * This function get Last value of throttle
 */
int16_t motorsGetThrottle();

/**
 * @brief Get Last value of steer
 *
 * This function get Last value of steer
 */
int16_t motorsGetSteer();

/**
 * @brief Gets the last applied target value for motor A.
 *
 * @return The last target value for motor A.
 */
uint32_t motorsGetLastTargetA();

/**
 * @brief Gets the last applied target value for motor B.
 *
 * @return The last target value for motor B.
 */
uint32_t motorsGetLastTargetB();
