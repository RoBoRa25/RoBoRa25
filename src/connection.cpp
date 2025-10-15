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
 * @file connection.cpp
 * @brief This file provides the implementation for WiFi connection functions.
 *
 * @details The functions in this file handle the low-level details of
 * connecting an ESP32 device to a WiFi network, including setting the
 * WiFi mode and waiting for a successful connection.
 */

#include "connection.h"

/* ====== Config di default ====== */

/// \brief flag to ensure event handlers are registered only once.
static bool s_eventsRegistered = false;

/**
 * @brief A static callback function for handling WiFi events.
 * @param event The type of WiFi event that occurred.
 * @details This function prints diagnostic messages to the serial console
 * based on the WiFi event. It handles events such as connection status,
 * IP acquisition, and client connections/disconnections in Access Point mode.
 */
static void onWifiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_WIFI_READY:
    DEBUG_PRINTLN("[WiFi] READY");
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    DEBUG_PRINTLN("[WiFi] STA START");
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    DEBUG_PRINTLN("[WiFi] STA CONNECTED");
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    DEBUG_PRINTF("[WiFi] STA GOT IP: %s", WiFi.localIP().toString().c_str());
    // wifiStartMdns();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    DEBUG_PRINTLN("[WiFi] STA DISCONNECTED");
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
    DEBUG_PRINTLN("[WiFi] AP START");
    // wifiStartMdns();
    break;
  case ARDUINO_EVENT_WIFI_AP_STOP:
    DEBUG_PRINTLN("[WiFi] AP STOP");
    break;
  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    DEBUG_PRINTLN("[WiFi] AP: client connected");
    break;
  case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    DEBUG_PRINTLN("[WiFi] AP: client disconnected");
    break;
  default:
    break;
  }
}

/* ====== Internal utilities ====== */
/**
 * @brief Waits for the device to connect in Station mode.
 * @param msTimeout The maximum time to wait in milliseconds.
 * @return `true` if the device connects within the timeout, `false` otherwise.
 * @details This is a blocking function that polls the connection status every
 * 200ms and prints a dot to the serial console for user feedback.
 */
static bool waitForStaConnect(unsigned long msTimeout)
{
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < msTimeout)
  {
    delay(200);
    DEBUG_PRINT('.');
  }
  return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Parses an IP address from a string, or uses a fallback if the string is invalid.
 * @param s The string containing the IP address to parse.
 * @param fallback The `IPAddress` object to use if parsing fails.
 * @return The parsed IP address or the fallback IP address.
 */
static IPAddress parseIpOr(const char *s, const IPAddress &fallback)
{
  IPAddress ip;
  if (s && ip.fromString(s))
    return ip;
  return fallback;
}

/**
 * @brief Applies common WiFi settings and registers event handlers.
 * @details This is an internal utility function that configures the device hostname,
 * disables power saving for better stability, and registers a global event handler
 * to listen for WiFi events.
 */
void wifiApplyTuningAndEvents()
{
  // Avoid flash writes for credentials
  WiFi.persistent(false);

  // Hostname (STA and AP)
  WiFi.setHostname(CONNECTION_HOSTNAME);

  // Power save off for greater stability/low latency
  WiFi.setSleep(false);

#ifdef CONNECTION_POLICY
  // Country EU per canali 1..13, policy auto
  wifi_country_t c = {"EU", 1, 13, 0, WIFI_COUNTRY_POLICY_AUTO};
  esp_wifi_set_country(&c);
#endif

  if (!s_eventsRegistered)
  {
    WiFi.onEvent(onWifiEvent);
    s_eventsRegistered = true;
  }
}

/**
 * @brief Starts the mDNS (multicast DNS) service.
 * @details This function begins the mDNS service with the device's hostname and
 * registers a service for HTTP on port 80.
 */
void wifiStartMdns()
{
  // mDNS
  if (!MDNS.begin(CONNECTION_HOSTNAME))
  {
    DEBUG_PRINTLN("[mDNS] start FAILED");
  }
  else
  {
    MDNS.addService("http", "tcp", 80);
    DEBUG_PRINTF("[mDNS] started as %s.local\n", CONNECTION_HOSTNAME);
  }
}

/**
 * @brief Connects the device in Station mode using provided credentials.
 * @param ssid The SSID of the target WiFi network.
 * @param pass The password for the target WiFi network.
 * @param retry The number of connection retries before giving up.
 * @return `true` if a connection is successfully established, `false` otherwise.
 */
bool wifiConnectSTA(const char *ssid, const char *pass, int retry)
{
  wifiApplyTuningAndEvents();

  if (!ssid || !*ssid)
  {
    DEBUG_PRINTLN("WiFi STA: SSID mancante");
    return false;
  }

  WiFi.mode(WIFI_STA);
  int attempt = 0;
  const unsigned long perAttemptTimeoutMs = CONNECTION_RETRY_TIMEOUT; // t per attempt

  do
  {
    DEBUG_PRINTF("WiFi (STA) tentativo %d: %s …\n", attempt + 1, ssid);
    WiFi.disconnect(true, true);
    delay(100);
    WiFi.begin(ssid, pass);

    if (waitForStaConnect(perAttemptTimeoutMs))
    {
      DEBUG_PRINTF("\nConnesso! IP: %s\n", WiFi.localIP().toString().c_str());
      // start service
      wifiStartMdns();
      return true;
    }

    DEBUG_PRINTLN("\nConnessione STA fallita.");
    attempt++;
    delay(300);
  } while (attempt <= retry);

  return false;
}

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
bool wifiStartAP(const char *ssid, const char *pass, const char *ip, const char *gw, const char *sub)
{
  wifiApplyTuningAndEvents();

  if (ssid == nullptr)
  {
    DEBUG_PRINTLN("WiFi AP: SSID mancante");
    return false;
  }

  if ((pass == nullptr) || (strlen(pass) < 8))
  {
    DEBUG_PRINTLN("WiFi AP: password troppo corta (min 8) – AP aperto");
  }

  IPAddress ipDef(192, 168, 4, 1);
  IPAddress gwDef(192, 168, 4, 1);
  IPAddress subDef(255, 255, 255, 0);

  IPAddress ipA = parseIpOr(ip, ipDef);
  IPAddress gwA = parseIpOr(gw, gwDef);
  IPAddress subA = parseIpOr(sub, subDef);

  WiFi.mode(WIFI_AP);
  bool cfgOk = WiFi.softAPConfig(ipA, gwA, subA);
  if (!cfgOk)
  {
    DEBUG_PRINTLN("WiFi AP: softAPConfig fallita – continuo con defaults.");
  }

  bool apOk = WiFi.softAP(ssid, (pass && strlen(pass) >= 8) ? pass : nullptr);
  if (!apOk)
  {
    DEBUG_PRINTLN("WiFi AP: softAP fallita");
    return false;
  }

  // Set hostname for AP too
#if defined(ARDUINO_ARCH_ESP32)
  WiFi.softAPsetHostname(CONNECTION_HOSTNAME);
#endif

  DEBUG_PRINTF("AP avviato: SSID=%s, IP=%s\n", ssid, WiFi.softAPIP().toString().c_str());

  // start service (mDNS)
  wifiStartMdns();
  return true;
}

/**
 * @brief A wrapper function that configures WiFi based on a `WiFiCfg` struct.
 * @param wCfg A struct containing all the necessary configuration parameters.
 * @return `true` if the selected WiFi mode is successfully configured, `false` otherwise.
 * @details This function checks the `wCfg.ap_sta` flag. If it's true, it starts
 * an Access Point. If false, it attempts to connect as a Station and, if that fails,
 * it falls back to starting an Access Point.
 */
bool wifiSetupFromParams(WiFiCfg wCfg)
{
  if (wCfg.ap_sta)
  {
    return wifiStartAP(wCfg.APssid, wCfg.APpass, wCfg.AP__ip, wCfg.AP__gw, wCfg.AP_sub);
  }
  else
  {
    if (wCfg.retray < 0)
      wCfg.retray = 0;
    if (wifiConnectSTA(wCfg.STssid, wCfg.STpass, wCfg.retray))
    {
      return true;
    }
    else
    {
      return wifiStartAP(wCfg.APssid, wCfg.APpass, wCfg.AP__ip, wCfg.AP__gw, wCfg.AP_sub);
    }
  }
}
