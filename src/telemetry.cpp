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
 * @file telemetry.cpp
 * @brief Implementation of the telemetry module.
 *
 * This file contains the logic for interacting with the IMU, collecting various sensor data,
 * and broadcasting it periodically over a network connection.
 */
#include "telemetry.h"

/// @brief The period in milliseconds between sensor tick.
uint8_t EnableTelemetry = 0;
/// @brief The period in milliseconds between sensor tick.
uint32_t SENSOR_PERIOD_MS = 100;
/// @brief Static instance of the IMU sensor.
static ROBORA_42670 IMU;
/// @brief Variable to store the latest IMU data frame.
_sRobOra_42670_IMU imuFrame;
/// @brief Timestamp for the last IMU update, used for rate limiting.
/// @brief Indicates whether the IMU initialization was successful
bool imuSuccessful;
/// @brief Indicates the IMU re-initialization
bool imuReinit = false;

/// @brief Battery voltage
float batteryVoltage = 0;

/**
 * @brief Initializes the I2C bus and the IMU sensor.
 *
 * Sets up the I2C communication on pins 5 (SDA) and 6 (SCL) for an ESP32-C3
 * and initializes the IMU at a frequency of 400kHz.
 */
void telemetryInit()
{
  TeleCfg cfg = configGetTeleCfg();
  SENSOR_PERIOD_MS = cfg.refresh;
  EnableTelemetry = cfg.enable;
  imuReinit = false;
  if (cfg.enable)
  {
    if (IMU.Init(Wire, true) == 0)
      imuSuccessful = true;
    else
      imuSuccessful = false;
    DEBUG_PRINTF("IMU initialization : %s\n", imuSuccessful ? "OK" : "KO");
  }

  /*Adc Configure*/
  analogReadResolution(12);       // 12 bit
  analogSetAttenuation(ADC_11db); // Set maximum attenuation (~2.5V max)
}

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
float telemetryReadAdC(uint8_t Pin)
{
  int raw_value = analogRead(Pin);
  float adc_voltage = ((float)raw_value / ADC_RESOLUTION) * MAX_ADC_VOLTAGE;
  return adc_voltage * VOLTAGE_DIVIDER_RATIO;
}

/**
 * @brief Re-Initializes the IMU and telemetry systems
 *
 * This function sets up the I2C bus and re-initializes the IMU sensor
 */
void telemetryReload()
{
  imuReinit = true;
}

/**
 * @brief Updates the IMU data at a fixed interval.
 *
 * This function is called frequently and updates the IMU data frame every 10 milliseconds
 * to ensure fresh readings.
 */
static void imuLoop()
{
  if (imuReinit)
  {
    telemetryInit();
    return;
  }

  batteryVoltage = telemetryReadAdC(PIN_BATTERY_VOLTAGE);

  if (!imuSuccessful)
    return;

  static uint32_t imuTickMs = 0;
  if ((millis() - imuTickMs) >= 10)
  {
    IMU.Loop();
    imuTickMs = millis();
    imuFrame = IMU.Get_ALL();
  }
}

String telemetryAddSensor(uint8_t position, String Value)
{
  return "\"sens" + String(position) + "\":\"" + Value + "\"";
}

/**
 * @brief Generates a complete JSON string with all sections (connection, motor, telemetry).
 * @return A JSON string compliant with the custom protocol (including the CMD key).
 */
String telemetrySensorString()
{
  uint8_t pos = 0;
  String jsonString = "{";
  jsonString += "\"CMD\":\"sensor\",";

  jsonString += telemetryAddSensor(pos++, String(imuFrame.Kal[0]));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String((-imuFrame.Kal[1])));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String(imuFrame.Kal[2]));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String(imuFrame.Temperature));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String(batteryVoltage));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String(0.0));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String(0.0));
  jsonString += ",";
  jsonString += telemetryAddSensor(pos++, String(millis()));
  jsonString += "}";
  return jsonString;
}

/**
 * @brief Broadcasts sensor data to connected clients.
 *
 * Creates a JSON document with various sensor readings (joystick, motor targets,
 * and IMU data like pitch, roll, and temperature) and broadcasts it to all
 * connected WebSocket clients.
 */
static void broadcastSensors()
{
  String s = telemetrySensorString();
  websocketAsyncMsg(s);
}

/**
 * @brief The main telemetry update loop.
 *
 * This function calls `imuLoop()` to update the IMU data. It also checks if the
 * sensor broadcast period has elapsed and calls `broadcastSensors()` to send
 * the latest data.
 */
void telemetryTick()
{

  static uint32_t lastSensorMs = 0;

  if (EnableTelemetry == 0)
    return;

  // aggiorna imu
  imuLoop();
  // telemetria periodica
  uint32_t now = millis();
  if (now - lastSensorMs >= SENSOR_PERIOD_MS)
  {
    lastSensorMs = now;
    broadcastSensors();
  }
}
