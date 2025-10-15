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

/*******************************************************
 * ROBORA PROJECT
 *******************************************************/

#include <Arduino.h>
#include "config.h"
#include "net.h"
#include "motors.h"
#include "ota.h"
#include "websocket.h"
#include "telemetry.h"
#include "connection.h"
#include "ledsrgb.h"
#include "functionkeys.h"
#include "display.h"


//#define DEMO_ROBOT_BASE
#define DEMO_ROBOT_TIMEOUT_WAITING    5000
#define DEMO_ROBOT_TIMEOUT_CONNECTED  200
extern const uint8_t RobotImage[DISPLAY_IMG_SIZE];
void PrintInfoOnDisplay();

void setup()
{
  Serial.begin(115200);
  DEBUG_PRINTLN("\nBooting…");

  /*-- Init config*/
  DEBUG_PRINTLN("LOAD CONFIG");
  configInit();

  /*-- LED NeoPixel --*/
  DEBUG_PRINTLN("LOAD LED");
  ledsInit(LEDRGB_NUMPIXELS, LEDRGB_PIN, LEDRGB_BRIGHTNESS);
  ledsOff();

  /*-- MOTORI / DRV8833 --*/
  DEBUG_PRINTLN("LOAD MOTOR");
  motorsInit();

  /*-- I2C --*/
  DEBUG_PRINTLN("LOAD I2C");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_SPEED);

  /*-- TELEMETRY --*/
  DEBUG_PRINTLN("LOAD TELEMETRY");
  telemetryInit();

  /*-- DISPLAY --*/
  DEBUG_PRINTLN("LOAD DISPLAY");
  displayBegin();
  PrintInfoOnDisplay();

  /*-- SPECIAL FUNCTION --*/
  DEBUG_PRINTLN("LOAD FN");
  fnInit();
  fnRegister(ledsOff);
  fnRegister(ledsON);
  fnRegister(ledsR);
  fnRegister(ledsG);
  fnRegister(ledsB);
  fnRegister(ledsSetRAINBOW);

  /*-- WI-FI --*/
  DEBUG_PRINTLN("LOAD WIFI");
  wifiSetupFromParams(configGetWiFiCfg());

  /*-- WEBSOCKET --*/
  DEBUG_PRINTLN("LOAD WEBSOCKET");
  mountWebSocket();

  /*--  ENDPOINTS OTA MULTIPART --*/
  DEBUG_PRINTLN("LOAD OTA MULTIPART");
  mountUpdateMultipart();

  /*--  ENDPOINTS OTA OCTET --*/
  DEBUG_PRINTLN("LOAD OTA OCTET");
  mountUpdateOctet();

  /*-- ASYNC SERVER HTTP --*/
  DEBUG_PRINTLN("LOAD HTTP");
  netInit();
}

void loop()
{
  /*-- SERVER CLIENT MANAGEMENT --*/
  netTick();

  /*-- TELEMETRY --*/
  telemetryTick();

  /*-- MOTOR COMMANDS --*/
  motorsTick();

  /*-- SPECIAL FUNCTION EXEC --*/
  fnExecuteTick();

  /*-- WEBSOCKET MANAGEMENT --*/
  websocketTick();

  /*-- INFO ON DISPALY --*/
  PrintInfoOnDisplay();

  /*-- DISPLAY MANAGEMENT --*/
  displaytick();
}

/*-- Print info on display if present --*/
void PrintInfoOnDisplay()
{

  WiFiCfg ConfigWifi;
  uint8_t NrString = 0;
  String buf[DISPLAY_MAX_LINES];
  static bool InfoOrImage = false;
  static unsigned long lastUpdate = 0;
  uint32_t timeoutChangeDisplay = DEMO_ROBOT_TIMEOUT_WAITING;
  bool AreClient;

  if (!displayEnable())
    return; // No display no party

  AreClient = websocketAreClients();
  if (AreClient)
  {
    timeoutChangeDisplay = DEMO_ROBOT_TIMEOUT_CONNECTED;
#ifndef DEMO_ROBOT_BASE
    return; /*Disable display for image and string from web */
#endif
  }

  if (millis() - lastUpdate > timeoutChangeDisplay)
  {
    if (AreClient)
    {
      /* Max 21 characters */
      /*****************"#####################*/
      buf[NrString++] = "CLIENT CONNECTED";
      buf[NrString++] = "MOTOR A :" + String(motorsGetLastTargetA());
      buf[NrString++] = "MOTOR B :" + String(motorsGetLastTargetA());
      buf[NrString++] = "THROTTLE:" + String(motorsGetThrottle());
      buf[NrString++] = "STEER   :" + String(motorsGetSteer());
      buf[NrString++] = "PITCH   :" + String(imuFrame.Kal[0]);
      buf[NrString++] = "ROLL    :" + String(imuFrame.Kal[1]);
      buf[NrString++] = "YAW     :" + String(imuFrame.Kal[2]);
      displayLoadAutoScroll(DISPLAY_SCROLL_MODE_NONE, buf, NrString, 1, 0, 1, 200, ((NrString > 8) ? 1 : 0));
    }
    else
    {
      if (InfoOrImage)
      {
        ConfigWifi = configGetWiFiCfg();
        /* Max 21 characters */
        /*****************"#####################*/
        buf[NrString++] = "MAKER FAIRE ROME 2025";
        buf[NrString++] = "ROBORA " + String(VERSIONE_APP) + "  BY ORAZIO";
        buf[NrString++] = "                     ";
        buf[NrString++] = padLeft("SSID:", String(ConfigWifi.APssid), 21, ' ');
        buf[NrString++] = padLeft("PASS:", String(ConfigWifi.APpass), 21, ' ');
        buf[NrString++] = padLeft("IP  :", String(ConfigWifi.AP__ip), 21, ' ');
        buf[NrString++] = padLeft("AGW :", String(ConfigWifi.AP__gw), 21, ' ');
        buf[NrString++] = padLeft("SUB :", String(ConfigWifi.AP_sub), 21, ' ');

        if (NrString > 8)
          displayLoadAutoScroll(DISPLAY_SCROLL_MODE_LINES, buf, NrString, 1, 0, 1, 200, ((NrString > 8) ? 1 : 0));
        else
          displayLoadAutoScroll(DISPLAY_SCROLL_MODE_NONE, buf, NrString, 1, 0, 1, 200, ((NrString > 8) ? 1 : 0));
      }
      else
      {
        displayLoadImage((uint8_t *)&RobotImage, 0, DISPLAY_IMG_SIZE);
      }
    }
    lastUpdate = millis();
    InfoOrImage ^= 1;
  }
  displaytick();
}