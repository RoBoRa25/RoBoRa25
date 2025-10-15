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
 * @file motors.cpp
 * @brief This file contains the implementation for controlling two motors using the RoBoRa_8833 library.
 *
 * It provides functions for initializing, configuring, and controlling the motors,
 * primarily for a robot or a similar mobile platform.
 */
#include "motors.h"

/**
 * @brief Configuration for motor A.
 */
RoBoRa_8833::MotorCfg cfgA{IN1A_PIN, IN2A_PIN, 0, 1, false};

/**
 * @brief Configuration for motor B.
 */

RoBoRa_8833::MotorCfg cfgB{IN1B_PIN, IN2B_PIN, 2, 3, false};

/**
 * @brief The motor control object.
 *
 * This instance of `RoBoRa_8833` manages both motors using the configurations `cfgA` and `cfgB`.
 */
RoBoRa_8833 motors(cfgA, cfgB);

/**
 * @brief Global variables to store joystick input values.
 *
 * These are `volatile` to ensure the compiler doesn't optimize away access,
 * as they can be modified by different parts of the code.
 */
volatile int16_t joyX = 0, joyY = 0;

/**
 * @brief Stores the timestamp of the last time motor commands were applied.
 *
 * Used to implement a a periodic update mechanism in `motorsTick()`.
 */
static uint32_t lastMoveApplyMs = 0;

/**
 * @brief Indicates the motors re-initialization.
 *
 * Used to implement a re-initilizazion the motors.
 */
bool motorsReinit = false;

/**
 * @brief Initializes the motors with configuration settings.
 *
 * This function reads motor configuration from `configGetMotoCfg()`,
 * sets up the motor controller, applies all the configuration parameters,
 * and sets the motors to a coast state.
 */
void motorsInit()
{
  MotoCfg cfg = configGetMotoCfg();
  motorsReinit = false;
  joyX = 0;
  joyY = 0;

  if (!motors.begin())
  {
    DEBUG_PRINTLN("Errore MOTORS (pin/Canali/freq non validi).");
    return;
  }

  motors.setMaxVel(cfg.maxVel);
  motors.setDeadzone(cfg.deadzone);
  motors.setExpoPct(cfg.expoPct);
  motors.setSteerGainPct(cfg.SteerGain);
  motors.setArcadeLvl(cfg.arcadeK);
  motors.setArcadeEn(cfg.arcadeEnabled);
  motors.setInvertiA(cfg.invertA);
  motors.setInvertiB(cfg.invertB);
  motors.setInvTankThr(cfg.tankInvThr);
  motors.setInvTankStr(cfg.tankInvStr);
  motors.coastA();
  motors.coastB();
  motorsPrintConfig();
}

/**
 * @brief Re-Initializesthe motor control system
 *
 * This function reads the motor configuration and sets up the hardware.
 */
void motorsReload(void)
{
  motorsReinit = true;
}

/**
 * @brief Applies new throttle and steer values to the motors.
 *
 * This function updates the global joystick variables and then commands the
 * motors to drive using a tank-style control scheme.
 *
 * @param throttle The throttle value, typically from -100 to 100.
 * @param steer The steering value, typically from -100 to 100.
 */
void motorsApply(int16_t throttle, int16_t steer)
{
  joyY = throttle;
  joyX = steer;
  motors.driveTank(joyY, joyX);
}

/**
 * @brief A periodic update function for the motors.
 *
 * This function checks if a certain amount of time has passed since the last
 * motor command was sent. If so, it calls `motorsApply()` to re-apply the
 * last known joystick values, ensuring a constant motor state.
 */
void motorsTick()
{
  if (motorsReinit)
  {
    motorsInit();
    return;
  }
  uint32_t now = millis();
  if (now - lastMoveApplyMs >= 100)
  {
    lastMoveApplyMs = now;
    motorsApply(joyY, joyX);
  }
}

/**
 * @brief Get Last value of throttle
 *
 * This function get Last value of throttle
 */
int16_t motorsGetThrottle(){return joyY;};

/**
 * @brief Get Last value of steer
 *
 * This function get Last value of steer
 */
int16_t motorsGetSteer(){return joyX;};

/**
 * @brief Gets the last target value set for motor A.
 *
 * @return The last target value for motor A.
 */
uint32_t motorsGetLastTargetA() { return motors.getLastTargtA(); }

/**
 * @brief Gets the last target value set for motor B.
 *
 * @return The last target value for motor B.
 */
uint32_t motorsGetLastTargetB() { return motors.getLastTargtB(); }


/**
 * @brief Prints the current motor configuration to the serial output.
 */
void motorsPrintConfig() { motors.printConfig(); }