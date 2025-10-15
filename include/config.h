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
 * @file config.h
 * @brief API and types for persistent configuration management (NVS) and parameter list generation.
 *
 * @details
 * This header defines the data types, configuration structures, and public functions to initialize, read, and write
 * application configuration parameters (motors, Wi-Fi, telemetry) to NVS. It also provides utilities to export
 * parameters in JSON format for a remote user interface.
 */

#pragma once
#include <Arduino.h>
#include "all_define.h"
#include "utility.h"

#include <Preferences.h>
#include <FS.h>
#ifdef CONFIG_PARTITION_USE_SPIFFS
#include <SPIFFS.h>
#else
#include <LittleFS.h>
#endif


/** @name NVS Namespace
 * @brief Strings identifying the NVS partitions/namespaces.
 */
#define MOTO_PREF_NS "moto_cfg"
#define WIFI_PREF_NS "wifi_cfg"
#define TELE_PREF_NS "tele_cfg"

/**
 * @enum ParamType
 * @brief Supported data types for persistent parameters.
 */
enum ParamType
{
    PARAM_TYPE_INT,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_STRING,
};

/**
 * @struct sParamDefaultValue
 * @brief Typed default value for a parameter.
 * @details
 * The internal union holds the default value, which is consistent with @ref ParamType.
 */
typedef struct sParamDefaultValue
{
    ParamType type;
    union
    {
        int int_val;
        float float_val;
        const char *str_val;
    };
} ParamDefaultValue;

/**
 * @union config_variant_t
 * @brief Variant to hold a generic runtime value.
 * @note For STRING, an internal 64-byte buffer is used.
 * @warning If a string parameter requires more than 63 characters + null terminator,
 * resize this buffer to be consistent with the overall usage.
 */
union config_variant_t
{
    int32_t int_val;
    float float_val;
    char str_val[64]; // Buffer per stringhe, la dimensione massima deve essere quella più grande tra tutti i parametri
};

/**
 * @struct sConfigValue
 * @brief Typed wrapper for dynamic values.
 */
typedef struct sConfigValue
{
    ParamType type;
    config_variant_t value;
} ConfigValue;

/**
 * @struct sParamInfo
 * @brief Descriptive metadata for a persistent parameter.
 *
 * @var sParamInfo::key NVS/JSON key (unique within its namespace).
 * @var sParamInfo::label User-friendly label (for UI).
 * @var sParamInfo::type Parameter type (@ref ParamType).
 * @var sParamInfo::min_val Minimum limit for INT/FLOAT (UI/validation).
 * @var sParamInfo::max_val Maximum limit for INT/FLOAT (UI/validation).
 * @var sParamInfo::max_len Maximum length (only for STRING, includes null terminator).
 * @var sParamInfo::default_val Default value (typed).
 */
typedef struct sParamInfo
{
    const char *key;
    const char *label;
    ParamType type;
    int min_val;
    int max_val;
    int max_len;
    ParamDefaultValue default_val;
} ParamInfo;

/**
 * @struct sMotoCfg
 * @brief Runtime configuration for motors/drive.
 */
typedef struct sMotoCfg
{
    uint8_t maxVel;
    uint8_t deadzone;
    uint8_t expoPct;
    uint8_t SteerGain;
    uint8_t arcadeK;
    bool arcadeEnabled;
    bool invertA;
    bool invertB;
    bool tankInvThr;
    bool tankInvStr;
} MotoCfg;

/**
 * @struct sWiFiCfg
 * @brief Runtime configuration for the Wi-Fi network.
 */
typedef struct sWiFiCfg
{
    bool ap_sta;
    uint8_t retray;
    char STssid[CONFIG_STRING_LEN];
    char STpass[CONFIG_STRING_LEN];
    char APssid[CONFIG_STRING_LEN];
    char APpass[CONFIG_STRING_LEN];
    char AP__ip[CONFIG_IPADD_LEN];
    char AP__gw[CONFIG_IPADD_LEN];
    char AP_sub[CONFIG_IPADD_LEN];
} WiFiCfg;

/**
 * @struct sTeleCfg
 * @brief Runtime configuration for telemetry.
 */
typedef struct sTeleCfg
{
    bool enable ;
    uint32_t refresh ;
} TeleCfg;

/**
 * @brief Initializes the filesystem, NVS namespaces, and loads/saves default values.
 * @post The runtime structures (MotoCfg, WiFiCfg, TeleCfg) are populated.
 */
void configInit();

/**
 * @brief Returns a copy of the motor configuration.
 * @return The current @ref MotoCfg structure.
 */
const MotoCfg configGetMotoCfg(void);

/**
 * @brief Returns a copy of the Wi-Fi configuration.
 * @return The current @ref WiFiCfg structure.
 */
const WiFiCfg configGetWiFiCfg(void);

/**
 * @brief Returns a copy of the telemetry configuration.
 * @return The current @ref TeleCfg structure.
 */
const TeleCfg configGetTeleCfg(void);

/**
 * @brief Generates a JSON string (without a library) with parameter metadata for a UI.
 * @return A JSON string with sections and fields (keys, labels, types, ...).
 * @note Designed for front-ends that build dynamic forms.
 */
String configGetListParameter();

/**
 * @brief Identifies the NVS namespace to which a parameter key belongs.
 * @param k The parameter key.
 * @return A constant string pointer to the namespace, or nullptr if not found.
 */
const char *configIsParamKey(const char *k);

/**
 * @brief Reads a string from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultValue A fallback value if the key does not exist.
 * @return The String with the read value or the default value.
 */
String configGet(const char *key, const String &defaultValue = "");

/**
 * @brief Reads an integer from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal A fallback value if the key does not exist.
 * @return The integer with the read value or the default value.
 */
int    configGetInt(const char *key, int defaultVal = 0);

/**
 * @brief Reads a boolean from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal A fallback value if the key does not exist.
 * @return The boolean with the read value or the default value.
 */
bool   configGetBool(const char *key, bool defaultVal = false);

/**
 * @brief Reads a float from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal A fallback value if the key does not exist.
 * @return The float with the read value or the default value.
 */
float  configGetFloat(const char *key, float defaultVal = 0.0f);

/**
 * @brief Reads a text string from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal A fallback value if the key does not exist.
 * @return The String with the read value or the default value.
 */
String configGetText(const char *key, const String &defaultVal = "");

/**
 * @brief Writes a string to the corresponding NVS namespace and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to save.
 * @return The number of bytes written to NVS.
 * @warning For STRING parameters, the function automatically truncates strings longer
 * than @ref sParamInfo::max_len.
 */
size_t configPut(const char *key, const String &value);

/**
 * @brief Writes an integer to the corresponding NVS namespace and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to save.
 * @return True if the write was successful, false otherwise.
 */
bool configPutInt(const char *key, int value);

/**
 * @brief Writes a boolean to the corresponding NVS namespace and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to save.
 * @return True if the write was successful, false otherwise.
 */
bool configPutBool(const char *key, bool value);

/**
 * @brief Writes a float to the corresponding NVS namespace and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to save.
 * @return True if the write was successful, false otherwise.
 */
bool configPutFloat(const char *key, float value);

/**
 * @brief Writes a text string to the corresponding NVS namespace and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to save.
 * @return True if the write was successful, false otherwise.
 */
bool configPutText(const char *key, const String &value);

/**
 * @brief Prints the current state of the configurations to the Serial port.
 */
void configPrintConfig(void);


/**
 * @brief Schedules a deferred device reboot.
 * @param ms The number of milliseconds to wait before rebooting.
 * @note Creates a low-priority task constrained to core 0.
 */
void configSaveAllDefaults();

