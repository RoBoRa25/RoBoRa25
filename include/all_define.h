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

#define VERSIONE_APP "1.0"

#define PINSPARE_GPIO7 7
#define PINSPARE_GPIO8 8
#define PINSPARE_GPIO9 9
#define PINSPARE_GPIO18 18
#define PINSPARE_GPIO19 19

// #define ROBORA_DEBUG_MODE // Commenta questa riga per disattivare il debug

#ifdef ROBORA_DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(x, ...)
#endif

/*---"System.h" --*/
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6
#define I2C_SPEED 400000

/*---ADC---*/
#define ADC_RESOLUTION 4096       // 12-bit resolution (0 a 4095)
#define MAX_ADC_VOLTAGE 2.5       // Maximum voltage measurable by the ADC with 11dB attenuation (circa 2.5V)
#define VOLTAGE_DIVIDER_RATIO 5.0 // Voltage divider ratio (es. 5.0 for 40kOhm and 10kOhm)
#define PIN_BATTERY_VOLTAGE 4

/*---"config.h" --*/
#define CONFIG_PARTITION_USE_SPIFFS

#define CONFIG_STRING_LEN 64
#define CONFIG_IPADD_LEN 16
#define MOTO_DEFAULT_MAXVEL 100
#define MOTO_DEFAULT_DEADZONE 5
#define MOTO_DEFAULT_EXPOPCT 0
#define MOTO_DEFAULT_STEERGAIN 70
#define MOTO_DEFAULT_ARCADEK 85
#define MOTO_DEFAULT_ARCADEENABLED 0
#define MOTO_DEFAULT_INVERTA 0
#define MOTO_DEFAULT_INVERTB 0
#define MOTO_DEFAULT_TANKINVTHR 0
#define MOTO_DEFAULT_TANKINVSTR 1

#define WIFI_DEFAULT_AP_STA 0
#define WIFI_DEFAULT_RETRAY 0
#define WIFI_DEFAULT_STSSID "MY_WIFI"
#define WIFI_DEFAULT_STPASS "MYPASSWORD"
#define WIFI_DEFAULT_APSSID "ROBORA2025"
#define WIFI_DEFAULT_APPASS "ROBORA2025"
#define WIFI_DEFAULT_AP__IP "192.168.4.1"
#define WIFI_DEFAULT_AP__GW "192.168.4.1"
#define WIFI_DEFAULT_AP_SUB "255.255.255.0"

#define TELE_DEFAULT_ENABLE 0
#define TELE_DEFAULT_REFRESH 250

/*---"net.h" --*/

/*---"motors.h" --*/
#define IN1A_PIN 0
#define IN2A_PIN 1
#define IN1B_PIN 2
#define IN2B_PIN 3
#define OUTSTOP_PIN 10 // NOT USED
/*---"ota.h" --*/
#define OTA_REQUEST_RESET 700

/*---"websocket.h" --*/
#define WS_REQUEST_RESET 500
#define WS_MAX_CLIENTS 4
#define WS_MAX_PAYLOAD (8 * 1024)

/*---"telemetry.h" --*/

/*---"connection.h" --*/

//!< The default hostname for the device, used in both STA and AP modes.
#define CONNECTION_HOSTNAME "RoBoRa"
//!< The timeout in milliseconds for a single connection attempt in station mode.
#define CONNECTION_RETRY_TIMEOUT 10000
//!< An optional macro to enable country policy settings for WiFi channels.
// #define CONNECTION_POLICY

/*---"ledsrgb.h" --*/
#define LEDRGB_PIN 8
#define LEDRGB_NUMPIXELS 1
#define LEDRGB_BRIGHTNESS 50

/*---"functionkeys.h" --*/
