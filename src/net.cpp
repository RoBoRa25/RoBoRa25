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
 * @file net.cpp
 * @brief Implementation file for network functionality.
 *
 * This file contains the definitions for the network server, WebSocket,
 * and their associated functions.
 */

#include "net.h"

/// @brief Global instance of the web server, initialized on port 80.
AsyncWebServer server(80);
/// @brief Global instance of the WebSocket, initialized with the "/ws" URL.
AsyncWebSocket ws("/ws");

/**
 * @brief Initializes the web server and its handlers.
 *
 * This function sets up the static file serving, adds a health check endpoint,
 * and starts the server. It determines the file system (SPIFFS or LittleFS)
 * based on the `CONFIG_PARTITION_USE_SPIFFS` macro.
 */
void netInit()
{

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

// Servi la UI statica (index.html)
#ifdef CONFIG_PARTITION_USE_SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("Index.html").setCacheControl("no-cache");
#else
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
#endif

  // Healthcheck
  server.on("/health", HTTP_GET, [](AsyncWebServerRequest *r)
            { r->send(200, "text/plain", "OK"); });

  server.on("/Robot3d.glb", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Robot3d.glb", "model/gltf-binary"); });

  if (displayEnable())
  {
    server.on("/upload_image", HTTP_POST, [](AsyncWebServerRequest *request)
              { request->send(200); }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
              {
                if (!displayLoadImageFromServer(filename, index, data, len, final))
                  request->send(500, "text/plain", "Error: Image too large"); });
  }
  else
  {
    server.on("/upload_image", HTTP_POST, [](AsyncWebServerRequest *request)
              { request->send(200); }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
              { request->send(500, "text/plain", "Error: Dispaly not enable"); });
  }

  // start server
  server.begin();
  DEBUG_PRINTLN("Server started.");
}

/**
 * @brief Performs periodic network tasks.
 *
 * @details This function is intended to be called in the main `loop()` to perform
 * necessary maintenance. Currently, it cleans up disconnected WebSocket clients
 * to free up resources.
 */
void netTick()
{
  ws.cleanupClients();
}