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
 * @file connection.h
 * @brief This file contains the declarations for WiFi connection management.
 *
 * @details This header file defines the interface for establishing a WiFi
 * connection using credentials from `config.h`. It includes a function
 * to configure the device in station mode and connect to a network.
 */

#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#ifdef ESP32
#include "esp_wifi.h"
#endif

#include "config.h"

/**
 * @brief Connects the device in Station mode using provided credentials.
 * @param ssid The SSID of the target WiFi network.
 * @param pass The password for the target WiFi network.
 * @param retry The number of connection retries before giving up.
 * @return `true` if a connection is successfully established, `false` otherwise.
 */
bool wifiConnectSTA(const char *ssid, const char *pass, int retry);

/**
 * @brief Starts the device in Access Point mode.
 * @param ssid The SSID for the new Access Point.
 * @param pass The password for the Access Point. A minimum of 8 characters is required.
 * If the password is too short or `nullptr`, the AP will be open.
 * @param ip The static IP address for the AP.
 * @param gw The gateway address for the AP.
 * @param sub The subnet mask for the AP.
 * @return `true` if the Access Point is successfully started, `false` otherwise.
 */
bool wifiStartAP(const char *ssid, const char *pass, const char *ip, const char *gw, const char *sub);

/**
 * @brief A wrapper function that configures WiFi based on a `WiFiCfg` struct.
 * @param wCfg A struct containing all the necessary configuration parameters.
 * @return `true` if the selected WiFi mode is successfully configured, `false` otherwise.
 * @details This function checks the `wCfg.ap_sta` flag. If it's true, it starts
 * an Access Point. If false, it attempts to connect as a Station and, if that fails,
 * it falls back to starting an Access Point.
 */
bool wifiSetupFromParams(WiFiCfg wCfg);

/**
 * @brief Applies common WiFi settings and registers event handlers.
 * @details This is an internal utility function that configures the device hostname,
 * disables power saving for better stability, and registers a global event handler
 * to listen for WiFi events.
 */
void wifiApplyTuningAndEvents();

/**
 * @brief Starts the mDNS (multicast DNS) service.
 * @details This function begins the mDNS service with the device's hostname and
 * registers a service for HTTP on port 80.
 */
void wifiStartMdns();
