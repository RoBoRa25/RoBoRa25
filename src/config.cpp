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
 * @file config.cpp
 * @brief Implementation of the configuration API based on NVS and JSON.
 * @details
 * This file contains the parameter lists with metadata (@ref ParamInfo), NVS initialization functions,
 * I/O methods, and functions to apply values to the runtime structures. It also includes utilities
 * for exporting metadata to JSON format.
 */
#include "config.h"

/// \brief Global handle to NVS preferences.
Preferences prefs;
/// \brief Runtime state: motor configuration.
MotoCfg motoCFG;
/// \brief Runtime state: Wi-Fi configuration.
WiFiCfg wifiCFG;
/// \brief Runtime state: telemetry configuration.
TeleCfg teleCFG;

/**
 * @brief List of motor parameter metadata.
 */
const ParamInfo motorsParamsList[] = {
    {"maxVel", "Velocità Massima", PARAM_TYPE_INT, 0, 100, 0, {PARAM_TYPE_INT, {.int_val = MOTO_DEFAULT_MAXVEL}}},
    {"deadzone", "Deadzone", PARAM_TYPE_INT, 0, 100, 0, {PARAM_TYPE_INT, {.int_val = MOTO_DEFAULT_DEADZONE}}},
    {"expoPct", "Expo Percentuale", PARAM_TYPE_INT, 0, 100, 0, {PARAM_TYPE_INT, {.int_val = MOTO_DEFAULT_EXPOPCT}}},
    {"SteerGain", "Guadagno Sterzo", PARAM_TYPE_INT, 0, 100, 0, {PARAM_TYPE_INT, {.int_val = MOTO_DEFAULT_STEERGAIN}}},
    {"arcadeK", "Arcade K", PARAM_TYPE_INT, 0, 100, 0, {PARAM_TYPE_INT, {.int_val = MOTO_DEFAULT_ARCADEK}}},
    {"arcadeEnabled", "Arcade Abilitato", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = MOTO_DEFAULT_ARCADEENABLED}}},
    {"invertA", "Inverti Motor A", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = MOTO_DEFAULT_INVERTA}}},
    {"invertB", "Inverti Motor B", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = MOTO_DEFAULT_INVERTB}}},
    {"tankInvThr", "Tank inverti Throttle", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = MOTO_DEFAULT_TANKINVTHR}}},
    {"tankInvStr", "Tank inverti Steer", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = MOTO_DEFAULT_TANKINVSTR}}},
};

/**
 * @brief List of Wi-Fi parameter metadata.
 */
const ParamInfo wifiParamsList[] = {
    {"ap_sta", "AccessPoint / Station", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = WIFI_DEFAULT_AP_STA}}},
    {"retray", "Retry Count", PARAM_TYPE_INT, 0, 100, 0, {PARAM_TYPE_INT, {.int_val = WIFI_DEFAULT_RETRAY}}},
    {"STssid", "Station SSID", PARAM_TYPE_STRING, 0, 0, CONFIG_STRING_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_STSSID}}},
    {"STpass", "Station Password", PARAM_TYPE_STRING, 0, 0, CONFIG_STRING_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_STPASS}}},
    {"APssid", "Access SSID", PARAM_TYPE_STRING, 0, 0, CONFIG_STRING_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_APSSID}}},
    {"APpass", "Access Password", PARAM_TYPE_STRING, 0, 0, CONFIG_STRING_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_APPASS}}},
    {"AP__ip", "IP address", PARAM_TYPE_STRING, 0, 0, CONFIG_IPADD_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_AP__IP}}},
    {"AP__gw", "Gateway address", PARAM_TYPE_STRING, 0, 0, CONFIG_IPADD_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_AP__GW}}},
    {"AP_sub", "Subnet address", PARAM_TYPE_STRING, 0, 0, CONFIG_IPADD_LEN, {PARAM_TYPE_STRING, {.str_val = WIFI_DEFAULT_AP_SUB}}},
};

/**
 * @brief List of telemetry parameter metadata.
 */
const ParamInfo telemetryParamsList[] = {
    {"enable", "Enable", PARAM_TYPE_BOOL, 0, 1, 0, {PARAM_TYPE_BOOL, {.int_val = TELE_DEFAULT_ENABLE}}},
    {"refresh", "Refersh Time", PARAM_TYPE_INT, 0, 3600, 0, {PARAM_TYPE_INT, {.int_val = TELE_DEFAULT_REFRESH}}},
};

/// \brief Number of motor parameters.
const int motorsCount = sizeof(motorsParamsList) / sizeof(motorsParamsList[0]);
/// \brief Number of Wi-Fi parameters.
const int wifiParamsCount = sizeof(wifiParamsList) / sizeof(wifiParamsList[0]);
/// \brief Number of telemetry parameters.
const int telemetryParamsCount = sizeof(telemetryParamsList) / sizeof(telemetryParamsList[0]);

/** @name Internal (Helper) Prototypes
 * @{ */
/// Restituisce i metadati di un parametro dato namespace e chiave.
const ParamInfo *configGetParamInfo(const char *pref_ns, const char *key);
/// Carica i valori dal NVS popolando le strutture runtime.
void configLoadFromNVS(const char *pref_ns, const ParamInfo *paramList, int paramCount);
/// Salva i valori di default sul NVS per il namespace specificato.
void configSaveDefaultsToNVS(const char *pref_ns, const ParamInfo *paramList, int paramCount);
/// Inizializza un namespace (prima esecuzione) o carica i valori salvati.
void configInitPreferencesByNamespace(const char *pref_ns, const ParamInfo *paramList, int paramCount);
/// Converte un @ref ParamType in stringa per uso UI.
const char *configParamTypeToString(ParamType type);
/// Appende al JSON la sezione descrittiva per un gruppo di parametri.
void configAppendSection(String &jsonString, const char *sectionName, const ParamInfo *params, size_t numParams);
/// Applica un valore già convertito alla struttura runtime corretta.
void configReloadValue(const ParamInfo *paramInfo, const ConfigValue &value);
/// Applica un valore espresso come stringa (da NVS o UI) alla struttura runtime.
void configApplyCFG(const char *pref_ns, const String &key, const String &val);
/** @} */

/**
 * @brief Initializes filesystem and preferences, then prints the configuration.
 * @note Mounts SPIFFS or LittleFS based on the build configuration.
 */
void configInit()
{
    // ---- Filesystem ----
#ifdef CONFIG_PARTITION_USE_SPIFFS
    if (!SPIFFS.begin(true))
        DEBUG_PRINTLN("SPIFFS mount failed");
#else
    if (!LittleFS.begin(true))
        DEBUG_PRINTLN("LittleFS mount failed");
#endif

    // NVS Management for Motors Configuration
    configInitPreferencesByNamespace(MOTO_PREF_NS, motorsParamsList, motorsCount);
    // NVS Management for WiFi Configuration
    configInitPreferencesByNamespace(WIFI_PREF_NS, wifiParamsList, wifiParamsCount);
    // NVS Management for telemetry Configuration
    configInitPreferencesByNamespace(TELE_PREF_NS, telemetryParamsList, telemetryParamsCount);

    // configPrintConfig();
}

/**
 * @brief Getter for a copy of the motor configuration.
 * @return @ref MotoCfg structure by value.
 */
const MotoCfg configGetMotoCfg(void) { return motoCFG; }

/**
 * @brief Getter for a copy of the Wi-Fi configuration.
 * @return @ref WiFiCfg structure by value.
 */
const WiFiCfg configGetWiFiCfg(void) { return wifiCFG; }

/**
 * @brief Getter for a copy of the telemetry configuration.
 * @return @ref TeleCfg structure by value.
 */
const TeleCfg configGetTeleCfg(void) { return teleCFG; }

/**
 * @brief Searches for a parameter's metadata given a namespace and key.
 * @param pref_ns NVS namespace (e.g., @ref MOTO_PREF_NS).
 * @param key The parameter key.
 * @return Pointer to @ref ParamInfo, or nullptr if not found.
 */
const ParamInfo *configGetParamInfo(const char *pref_ns, const char *key)
{
    if (strcmp(pref_ns, MOTO_PREF_NS) == 0)
    {
        for (int i = 0; i < motorsCount; ++i)
        {
            if (strcmp(motorsParamsList[i].key, key) == 0)
                return &motorsParamsList[i];
        }
    }
    else if (strcmp(pref_ns, WIFI_PREF_NS) == 0)
    {
        for (int i = 0; i < wifiParamsCount; ++i)
        {
            if (strcmp(wifiParamsList[i].key, key) == 0)
                return &wifiParamsList[i];
        }
    }
    else if (strcmp(pref_ns, TELE_PREF_NS) == 0)
    {
        for (int i = 0; i < telemetryParamsCount; ++i)
        {
            if (strcmp(telemetryParamsList[i].key, key) == 0)
                return &telemetryParamsList[i];
        }
    }
    return nullptr;
}

/**
 * @brief Loads values from NVS and applies them to the runtime structures.
 * @param pref_ns The reference namespace.
 * @param paramList The parameter metadata table.
 * @param paramCount The number of elements in @p paramList.
 * @post The global structures (motoCFG, wifiCFG, teleCFG) are updated.
 * @note If a key does not exist, it is created with the default value.
 */
void configLoadFromNVS(const char *pref_ns, const ParamInfo *paramList, int paramCount)
{
    if (!prefs.begin(pref_ns, /* readOnly = */ true))
    {
        DEBUG_PRINTF("Errore: impossibile aprire NVS per '%s'.\n", pref_ns);
        return;
    }

    for (int i = 0; i < paramCount; ++i)
    {
        const ParamInfo &p = paramList[i];
        String v;

        // If the key does not exist, create the default value
        if (!prefs.isKey(p.key))
        {
            switch (p.type)
            {
            case PARAM_TYPE_INT:
            case PARAM_TYPE_BOOL:
                prefs.putInt(p.key, p.default_val.int_val);
                break;
            case PARAM_TYPE_FLOAT:
                prefs.putFloat(p.key, p.default_val.float_val);
                break;
            case PARAM_TYPE_STRING:
                prefs.putString(p.key, p.default_val.str_val ? p.default_val.str_val : "");
                break;
            }
            DEBUG_PRINTF("Chiave '%s' non trovata. Salvato valore di default.\n", p.key);
        }

        // Reading by type
        switch (p.type)
        {
        case PARAM_TYPE_INT:
        case PARAM_TYPE_BOOL:
        {
            int val = prefs.getInt(p.key, p.default_val.int_val);
            v = String(val);
            break;
        }
        case PARAM_TYPE_FLOAT:
        {
            float val = prefs.getFloat(p.key, p.default_val.float_val);
            v = String(val);
            break;
        }
        case PARAM_TYPE_STRING:
        {
            v = prefs.getString(p.key, p.default_val.str_val ? p.default_val.str_val : "");
            if (p.max_len > 0 && v.length() > (size_t)(p.max_len - 1))
            {
                DEBUG_PRINTF("Warning: valore STRING per '%s' troppo lungo (%u > %d). Troncato e risalvato.\n", p.key, (unsigned)v.length(), p.max_len - 1);
                v = v.substring(0, p.max_len - 1);
                prefs.putString(p.key, v);
            }
            break;
        }
        }
        if (!v.isEmpty())
        {
            // DEBUG_PRINTF("KEY: '%16s' Valore:%s\n", p.key, v);
            configApplyCFG(pref_ns, p.key, v);
        }
    }

    prefs.end();
}

/**
 * @brief Writes the default values to NVS for a specified namespace and marks it as initialized.
 * @param pref_ns The reference namespace.
 * @param paramList The metadata table.
 * @param paramCount The number of elements.
 */
void configSaveDefaultsToNVS(const char *pref_ns, const ParamInfo *paramList, int paramCount)
{
    prefs.begin(pref_ns, false);
    for (int i = 0; i < paramCount; ++i)
    {
        const ParamInfo *paramInfo = &paramList[i];
        switch (paramInfo->type)
        {
        case PARAM_TYPE_INT:
        case PARAM_TYPE_BOOL:
            prefs.putInt(paramInfo->key, paramInfo->default_val.int_val);
            break;
        case PARAM_TYPE_FLOAT:
            prefs.putFloat(paramInfo->key, paramInfo->default_val.float_val);
            break;
        case PARAM_TYPE_STRING:
            prefs.putString(paramInfo->key, paramInfo->default_val.str_val);
            break;
        default:
            break;
        }
    }
    prefs.putBool("initialized", true);
    prefs.end();
}

/**
 * @brief Initializes or loads a preferences namespace.
 * @param pref_ns The namespace (@ref MOTO_PREF_NS, @ref WIFI_PREF_NS, @ref TELE_PREF_NS).
 * @param paramList Parameter metadata.
 * @param paramCount The number of elements.
 */
void configInitPreferencesByNamespace(const char *pref_ns, const ParamInfo *paramList, int paramCount)
{
    bool Initialized = false;

    prefs.begin(pref_ns, false);
    Initialized = prefs.getBool("initialized", false);
    prefs.end();

    if (!Initialized)
    {
        DEBUG_PRINTF("Initializing %s config for the first time...\n", pref_ns);
        configSaveDefaultsToNVS(pref_ns, paramList, paramCount);
    }
    else
    {
        DEBUG_PRINTF("Loading %s config from NVS...\n", pref_ns);
        configLoadFromNVS(pref_ns, paramList, paramCount);
    }
}

/**
 * @brief Determines which namespace a key belongs to.
 * @param k The parameter key.
 * @return String with the namespace or nullptr if unknown.
 * @bug Returns @ref WIFI_PREF_NS even for TELE keys (likely a typo).
 */
const char *configIsParamKey(const char *k)
{
    if (configGetParamInfo(MOTO_PREF_NS, k) != nullptr)
        return MOTO_PREF_NS;
    if (configGetParamInfo(WIFI_PREF_NS, k) != nullptr)
        return WIFI_PREF_NS;
    if (configGetParamInfo(TELE_PREF_NS, k) != nullptr)
        return TELE_PREF_NS;
    return nullptr;
}

/**
 * @brief Reads a string from NVS for the specified key (deriving the namespace).
 * @param key The parameter key.
 * @param defaultValue A fallback value if the key does not exist.
 * @return The string read from NVS.
 */
String configGet(const char *key, const String &defaultValue)
{
    const char *ns = configIsParamKey(key);
    if (!ns)
    {
        DEBUG_PRINTF("Config_rd: chiave '%s' sconosciuta.\n", key);
        return defaultValue;
    }

    const ParamInfo *pi = configGetParamInfo(ns, key);
    if (!pi)
    {
        DEBUG_PRINTF("Config_rd: metadati mancanti per '%s'.\n", key);
        return defaultValue;
    }

    if (!prefs.begin(ns, /*readOnly=*/true))
    {
        DEBUG_PRINTF("Config_rd: impossibile aprire NVS per '%s'.\n", ns);
        return defaultValue;
    }

    // Se la chiave non esiste in NVS, torna il default passato
    if (!prefs.isKey(key))
    {
        prefs.end();
        return defaultValue;
    }

    String out;
    switch (pi->type)
    {
    case PARAM_TYPE_INT:
    case PARAM_TYPE_BOOL:
    {
        int v = prefs.getInt(key, pi->default_val.int_val);
        out = String(v);
        break;
    }
    case PARAM_TYPE_FLOAT:
    {
        float f = prefs.getFloat(key, pi->default_val.float_val);
        out = String(f);
        break;
    }
    case PARAM_TYPE_STRING:
    {
        out = prefs.getString(key, pi->default_val.str_val ? pi->default_val.str_val : "");
        if (pi->max_len > 0 && out.length() > (size_t)(pi->max_len - 1))
        {
            out = out.substring(0, pi->max_len - 1);
        }
        break;
    }
    }

    prefs.end();
    DEBUG_PRINTF("Config_rd: [%s] %s = %s\n", ns, key, out.c_str());
    return out;
}

/**
 * @brief Reads an integer from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal The fallback value.
 * @return The integer value.
 */
int configGetInt(const char *key, int defaultVal)
{
    const char *ns = configIsParamKey(key);
    const ParamInfo *pi = ns ? configGetParamInfo(ns, key) : nullptr;
    if (!ns || !pi)
        return defaultVal;
    if (!prefs.begin(ns, true))
        return defaultVal;
    int v = prefs.isKey(key) ? prefs.getInt(key, pi->default_val.int_val) : defaultVal;
    prefs.end();
    return v;
}

/**
 * @brief Reads a boolean from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal The fallback value.
 * @return The boolean value.
 */
bool configGetBool(const char *key, bool defaultVal)
{
    return configGetInt(key, defaultVal ? 1 : 0) != 0;
}

/**
 * @brief Reads a float from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal The fallback value.
 * @return The float value.
 */
float configGetFloat(const char *key, float defaultVal)
{
    const char *ns = configIsParamKey(key);
    const ParamInfo *pi = ns ? configGetParamInfo(ns, key) : nullptr;
    if (!ns || !pi)
        return defaultVal;
    if (!prefs.begin(ns, true))
        return defaultVal;
    float f = prefs.isKey(key) ? prefs.getFloat(key, pi->default_val.float_val) : defaultVal;
    prefs.end();
    return f;
}

/**
 * @brief Reads a string from the corresponding NVS namespace.
 * @param key The parameter key.
 * @param defaultVal The fallback value.
 * @return The string value.
 */
String configGetText(const char *key, const String &defaultVal)
{
    const char *ns = configIsParamKey(key);
    const ParamInfo *pi = ns ? configGetParamInfo(ns, key) : nullptr;
    if (!ns || !pi)
        return defaultVal;
    if (!prefs.begin(ns, true))
        return defaultVal;
    String s = prefs.isKey(key) ? prefs.getString(key, pi->default_val.str_val ? pi->default_val.str_val : "") : defaultVal;
    prefs.end();
    if (pi->max_len > 0 && s.length() > (size_t)(pi->max_len - 1))
    {
        s = s.substring(0, pi->max_len - 1);
    }
    return s;
}

/**
 * @brief Writes a string to NVS (with length check) and applies to runtime.
 * @param key The parameter key.
 * @param value The value to write.
 * @return The number of bytes written.
 */
size_t configPut(const char *key, const String &value)
{
    const char *ptr_pref_ns = configIsParamKey(key);
    if (!ptr_pref_ns)
    {
        DEBUG_PRINTF("Config_wr: chiave '%s' sconosciuta.\n", key);
        return 0;
    }

    const ParamInfo *paramInfo = configGetParamInfo(ptr_pref_ns, key);
    if (!paramInfo)
    {
        DEBUG_PRINTF("Config_wr: metadati mancanti per '%s'.\n", key);
        return 0;
    }

    size_t written = 0;
    String appliedStr; 
    // Open NVS for writing only once
    if (!prefs.begin(ptr_pref_ns, /*readOnly=*/false))
    {
        DEBUG_PRINTF("Config_wr: impossibile aprire NVS per '%s'.\n", ptr_pref_ns);
        return 0;
    }

    switch (paramInfo->type)
    {
    case PARAM_TYPE_INT:
    {
        long v = value.toInt();
        if (paramInfo->min_val < paramInfo->max_val)
        {
            if (v < paramInfo->min_val)
                v = paramInfo->min_val;
            if (v > paramInfo->max_val)
                v = paramInfo->max_val;
        }
        written = prefs.putInt(key, (int)v);
        appliedStr = String((int)v);
        break;
    }

    case PARAM_TYPE_BOOL:
    {
        // accept 1/0, true/false, on/off, yes/no
        bool b = false;
        if (value.equalsIgnoreCase("1") || value.equalsIgnoreCase("true") ||
            value.equalsIgnoreCase("on") || value.equalsIgnoreCase("yes"))
        {
            b = true;
        }
        written = prefs.putInt(key, b);
        appliedStr = b ? "1" : "0";
        break;
    }

    case PARAM_TYPE_FLOAT:
    {
        float f = value.toFloat();
        if (paramInfo->min_val < paramInfo->max_val)
        {
            if (f < (float)paramInfo->min_val)
                f = (float)paramInfo->min_val;
            if (f > (float)paramInfo->max_val)
                f = (float)paramInfo->max_val;
        }
        written = prefs.putFloat(key, f);
        appliedStr = String(f);
        break;
    }

    case PARAM_TYPE_STRING:
    {
        String v = value;
        if (paramInfo->max_len > 0 && v.length() > (size_t)(paramInfo->max_len - 1))
        {
            DEBUG_PRINTF("Warning: stringa per '%s' troppo lunga (%u > %d). Troncata.\n", key, (unsigned)v.length(), paramInfo->max_len - 1);
            v = v.substring(0, paramInfo->max_len - 1);
        }
        written = prefs.putString(key, v);
        appliedStr = v;
        break;
    }
    }

    prefs.end();

    // It also applies to runtime if we have written something
    if (written > 0 && appliedStr.length() > 0)
    {
        configApplyCFG(ptr_pref_ns, String(key), appliedStr);
        DEBUG_PRINTF("Config_wr: [%s] %s = %s (bytes=%u)\n", ptr_pref_ns, key, appliedStr.c_str(), (unsigned)written);
    }

    return written;
}

/**
 * @brief Writes an integer to NVS and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to write.
 * @return True if successful, false otherwise.
 */
bool configPutInt(const char *key, int value)
{
    const char *ns = configIsParamKey(key);
    if (!ns)
        return false;
    const ParamInfo *pi = configGetParamInfo(ns, key);
    if (!pi || (pi->type != PARAM_TYPE_INT && pi->type != PARAM_TYPE_BOOL))
        return false;

    // Clamp range
    if (pi->min_val < pi->max_val)
    {
        if (value < pi->min_val)
            value = pi->min_val;
        if (value > pi->max_val)
            value = pi->max_val;
    }

    if (!prefs.begin(ns, false))
        return false;
    prefs.putInt(key, value);
    prefs.end();

    configApplyCFG(ns, String(key), String(value));
    return true;
}

/**
 * @brief Writes a boolean to NVS and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to write.
 * @return True if successful, false otherwise.
 */
bool configPutBool(const char *key, bool value)
{
    return configPutInt(key, value ? 1 : 0);
}

/**
 * @brief Writes a float to NVS and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to write.
 * @return True if successful, false otherwise.
 */
bool configPutFloat(const char *key, float value)
{
    const char *ns = configIsParamKey(key);
    if (!ns)
        return false;
    const ParamInfo *pi = configGetParamInfo(ns, key);
    if (!pi || pi->type != PARAM_TYPE_FLOAT)
        return false;

    // Clamp range usando min_val/max_val
    if (pi->min_val < pi->max_val)
    {
        if (value < (float)pi->min_val)
            value = (float)pi->min_val;
        if (value > (float)pi->max_val)
            value = (float)pi->max_val;
    }

    if (!prefs.begin(ns, false))
        return false;
    prefs.putFloat(key, value);
    prefs.end();

    configApplyCFG(ns, String(key), String(value));
    return true;
}

/**
 * @brief Writes a text string to NVS and applies the value at runtime.
 * @param key The parameter key.
 * @param value The value to write.
 * @return True if successful, false otherwise.
 */
bool configPutText(const char *key, const String &value)
{
    const char *ns = configIsParamKey(key);
    if (!ns)
        return false;
    const ParamInfo *pi = configGetParamInfo(ns, key);
    if (!pi || pi->type != PARAM_TYPE_STRING)
        return false;

    String v = value;
    if (pi->max_len > 0 && v.length() > (size_t)(pi->max_len - 1))
    {
        DEBUG_PRINTF("Warning: stringa per '%s' troppo lunga (%u > %d). Troncata.\n", key, (unsigned)v.length(), pi->max_len - 1);
        v = v.substring(0, pi->max_len - 1);
    }

    if (!prefs.begin(ns, false))
        return false;
    prefs.putString(key, v);
    prefs.end();

    configApplyCFG(ns, String(key), v);
    return true;
}

/**
 * @brief Converts a parameter type to an HTML input type string (for UI).
 * @param type The parameter type.
 * @return "number", "checkbox", or "text".
 */
const char *configParamTypeToString(ParamType type)
{
    switch (type)
    {
    case PARAM_TYPE_INT:
        return "number";
    case PARAM_TYPE_BOOL:
        return "checkbox";
    case PARAM_TYPE_FLOAT:
        return "number";
    case PARAM_TYPE_STRING:
        return "text";
    }
    return "text";
}

/**
 * @brief Appends a parametric section (keys, labels, types) to a JSON string.
 * @param jsonString The destination JSON string.
 * @param sectionName The name of the section.
 * @param params The metadata array.
 * @param numParams The number of elements.
 */
void configAppendSection(String &jsonString, const char *sectionName, const ParamInfo *params, size_t numParams)
{
    jsonString += "\"" + String(sectionName) + "\":{";

    // Adds the list of keys
    jsonString += "\"params\":[";
    for (size_t i = 0; i < numParams; i++)
    {
        jsonString += "\"" + String(params[i].key) + "\"";
        if (i < numParams - 1)
            jsonString += ",";
    }
    jsonString += "],";

    // Adds the list of labels
    jsonString += "\"labels\":[";
    for (size_t i = 0; i < numParams; i++)
    {
        jsonString += "\"" + String(params[i].label) + "\"";
        if (i < numParams - 1)
            jsonString += ",";
    }
    jsonString += "],";

    // Adds the type list
    jsonString += "\"types\":[";
    for (size_t i = 0; i < numParams; i++)
    {
        jsonString += "\"" + String(configParamTypeToString(params[i].type)) + "\"";
        if (i < numParams - 1)
            jsonString += ",";
    }
    jsonString += "]";
    jsonString += "}";
}

/**
 * @brief Generates a complete JSON string with all sections (connection, motor, telemetry).
 * @return A JSON string compliant with the custom protocol (including the CMD key).
 */
String configGetListParameter()
{
    String jsonString = "{";
    jsonString += "\"CMD\":\"config_req\",";
    configAppendSection(jsonString, "connessione", wifiParamsList, wifiParamsCount);
    jsonString += ",";
    configAppendSection(jsonString, "motore", motorsParamsList, motorsCount);
    jsonString += ",";
    configAppendSection(jsonString, "telemetria", telemetryParamsList, telemetryParamsCount);
    jsonString += "}";
    return jsonString;
}

/**
 * @brief Applies a typed value to the correct runtime structure based on the key.
 * @param paramInfo The parameter's metadata.
 * @param value The typed value to apply.
 */
void configReloadValue(const ParamInfo *paramInfo, const ConfigValue &value)
{
    if (!paramInfo)
        return;

    if (strcmp(paramInfo->key, "maxVel") == 0)
        motoCFG.maxVel = constrain(value.value.int_val, paramInfo->min_val, paramInfo->max_val);
    else if (strcmp(paramInfo->key, "deadzone") == 0)
        motoCFG.deadzone = constrain(value.value.int_val, paramInfo->min_val, paramInfo->max_val);
    else if (strcmp(paramInfo->key, "expoPct") == 0)
        motoCFG.expoPct = constrain(value.value.int_val, paramInfo->min_val, paramInfo->max_val);
    else if (strcmp(paramInfo->key, "SteerGain") == 0)
        motoCFG.SteerGain = constrain(value.value.int_val, paramInfo->min_val, paramInfo->max_val);
    else if (strcmp(paramInfo->key, "arcadeK") == 0)
        motoCFG.arcadeK = constrain(value.value.int_val, paramInfo->min_val, paramInfo->max_val);
    else if (strcmp(paramInfo->key, "arcadeEnabled") == 0)
        motoCFG.arcadeEnabled = value.value.int_val;
    else if (strcmp(paramInfo->key, "invertA") == 0)
        motoCFG.invertA = value.value.int_val;
    else if (strcmp(paramInfo->key, "invertB") == 0)
        motoCFG.invertB = value.value.int_val;
    else if (strcmp(paramInfo->key, "tankInvThr") == 0)
        motoCFG.tankInvThr = value.value.int_val;
    else if (strcmp(paramInfo->key, "tankInvStr") == 0)
        motoCFG.tankInvStr = value.value.int_val;

    else if (strcmp(paramInfo->key, "ap_sta") == 0)
        wifiCFG.ap_sta = value.value.int_val;
    else if (strcmp(paramInfo->key, "retray") == 0)
        wifiCFG.retray = value.value.int_val;
    else if (strcmp(paramInfo->key, "STssid") == 0)
        strncpy(wifiCFG.STssid, value.value.str_val, paramInfo->max_len - 1);
    else if (strcmp(paramInfo->key, "STpass") == 0)
        strncpy(wifiCFG.STpass, value.value.str_val, paramInfo->max_len - 1);
    else if (strcmp(paramInfo->key, "APssid") == 0)
        strncpy(wifiCFG.APssid, value.value.str_val, paramInfo->max_len - 1);
    else if (strcmp(paramInfo->key, "APpass") == 0)
        strncpy(wifiCFG.APpass, value.value.str_val, paramInfo->max_len - 1);
    else if (strcmp(paramInfo->key, "AP__ip") == 0)
        strncpy(wifiCFG.AP__ip, value.value.str_val, paramInfo->max_len - 1);
    else if (strcmp(paramInfo->key, "AP__gw") == 0)
        strncpy(wifiCFG.AP__gw, value.value.str_val, paramInfo->max_len - 1);
    else if (strcmp(paramInfo->key, "AP_sub") == 0)
        strncpy(wifiCFG.AP_sub, value.value.str_val, paramInfo->max_len - 1);

    else if (strcmp(paramInfo->key, "enable") == 0)
        teleCFG.enable = value.value.int_val;
    else if (strcmp(paramInfo->key, "refresh") == 0)
        teleCFG.refresh = value.value.int_val;
};

/**
 * @brief Applies a key/value pair (string) obtained from NVS or a UI.
 * @param pref_ns The relevant namespace.
 * @param key The parameter key.
 * @param val The value as a string (will be converted based on @ref ParamInfo::type).
 */
void configApplyCFG(const char *pref_ns, const String &key, const String &val)
{
    const ParamInfo *paramInfo = configGetParamInfo(pref_ns, key.c_str());
    if (paramInfo == nullptr)
        return;

    ConfigValue newValue;
    newValue.type = paramInfo->type;

    switch (paramInfo->type)
    {
    case PARAM_TYPE_INT:
    case PARAM_TYPE_BOOL:
        newValue.value.int_val = val.toInt();
        break;
    case PARAM_TYPE_FLOAT:
        newValue.value.float_val = val.toFloat();
        break;
    case PARAM_TYPE_STRING:
    {
        strncpy(newValue.value.str_val, val.c_str(), sizeof(newValue.value.str_val) - 1);
        newValue.value.str_val[sizeof(newValue.value.str_val) - 1] = '\0';
        break;
    }
    default:
        return;
    }
    configReloadValue(paramInfo, newValue);
}

/**
 * @brief Prints a diagnostic of the current parameters to the Serial port.
 */
void configPrintConfig(void)
{
    DEBUG_PRINTF("MOTO CFG: %d parametri \n", motorsCount);
    DEBUG_PRINTF("maxVel: %d deadzone: %d expoPct: %d SteerGain: %d  \n", motoCFG.maxVel, motoCFG.deadzone, motoCFG.expoPct, motoCFG.SteerGain);
    DEBUG_PRINTF("arcadeK: %d arcadeEnabled: %d  \n", motoCFG.arcadeK, motoCFG.arcadeEnabled);
    DEBUG_PRINTF("invMotA: %d invMotA: %d tankInvThr: %d tankInvStr: %d \n", motoCFG.invertA, motoCFG.invertB, motoCFG.tankInvThr, motoCFG.tankInvStr);
    DEBUG_PRINTF("WIFI CFG: %d parametri \n", wifiParamsCount);
    DEBUG_PRINTF("Type - %s Retry:%d \n", wifiCFG.ap_sta ? "STA" : "AP", wifiCFG.retray);
    DEBUG_PRINTF("ST   - SSID: %s PASS:%s \n", wifiCFG.STssid, wifiCFG.STpass);
    DEBUG_PRINTF("AP   - SSID: %s PASS:%s \n", wifiCFG.APssid, wifiCFG.APpass);
    DEBUG_PRINTF("       IP: %s GW:%s SU:%s \n", wifiCFG.AP__ip, wifiCFG.AP__gw, wifiCFG.AP_sub);
    DEBUG_PRINTF("TELE CFG: %d parametri \n", telemetryParamsCount);
    DEBUG_PRINTF("Enable - %s Retry:%d \n", teleCFG.enable ? "ON " : "OFF", teleCFG.refresh);
}

/**
 * @brief Restores all default values in NVS and updates the runtime structures.
 * @note Overwrites any custom values.
 */
void configSaveAllDefaults()
{
    DEBUG_PRINTLN(">>> Ripristino di TUTTI i valori di default <<<");

    // Save and update MOTO
    configSaveDefaultsToNVS(MOTO_PREF_NS, motorsParamsList, motorsCount);
    configLoadFromNVS(MOTO_PREF_NS, motorsParamsList, motorsCount);

    // Save and update WIFI
    configSaveDefaultsToNVS(WIFI_PREF_NS, wifiParamsList, wifiParamsCount);
    configLoadFromNVS(WIFI_PREF_NS, wifiParamsList, wifiParamsCount);

    // Save and update TELEMETRIA
    configSaveDefaultsToNVS(TELE_PREF_NS, telemetryParamsList, telemetryParamsCount);
    configLoadFromNVS(TELE_PREF_NS, telemetryParamsList, telemetryParamsCount);

    // configPrintConfig();
    DEBUG_PRINTLN(">>> Default ripristinati con successo <<<");
}
