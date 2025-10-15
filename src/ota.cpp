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
 * @file ota.cpp
 * @brief This file contains the implementation for Over-The-Air (OTA) update functions.
 *
 * Implementation the main interfaces for mounting HTTP endpoints dedicated to
 * updating the device's firmware or filesystem.
 */
#include "ota.h"

/// @brief Total number of bytes written during the OTA upload.
static size_t written = 0;      
/// @brief Timestamp of the last progress update sent via WebSocket for rate limiting.
static uint32_t lastProgMs = 0; // La variabile globale

/**
 * @brief Sends an OTA message to all connected WebSocket clients.
 *
 * This function serializes a JSON document and broadcasts it to all clients
 * connected to the `ws` server.
 *
 * @param fill A lambda function that populates the JSON document with the desired fields.
 */
static void wsBroadcastOta(std::function<void(JsonDocument &)> fill)
{
  JsonDocument doc;
  doc["CMD"] = "ota";
  fill(doc);
  String s;
  serializeJson(doc, s);
  ws.textAll(s);
}

/**
 * @brief Notifies WebSocket clients about the start of an OTA update.
 *
 * @param isFs True if the target is the filesystem partition, false for firmware.
 * @param total The total size of the file to be written.
 * @param max The maximum size of the destination partition.
 * @param label The name of the destination partition.
 */
static void wsOtaStart(bool isFs, size_t total, size_t max, const char *label)
{
  wsBroadcastOta([&](JsonDocument &d)
                 {
    d["event"]="start"; d["target"]= isFs?"fs":"app";
    d["total"]=total; d["max"]=max; if(label) d["part"]=label; });
}

/**
 * @brief Notifies WebSocket clients about a rejected OTA update due to a preliminary error.
 *
 * @param reason A string describing the reason for the rejection.
 */
static void wsOtaReject(const String &reason)
{
  wsBroadcastOta([&](JsonDocument &d)
                 { d["event"]="reject"; d["reason"]=reason; });
}

/**
 * @brief Notifies WebSocket clients about OTA update progress.
 *
 * @param done The number of bytes written so far.
 * @param total The total number of expected bytes.
 */
static void wsOtaProgress(size_t done, size_t total)
{
  wsBroadcastOta([&](JsonDocument &d)
                 { d["event"]="progress"; d["done"]=done; d["total"]=total; });
}

/**
 * @brief Notifies WebSocket clients about the completion of an OTA update.
 *
 * @param ok True if the update was successful.
 * @param msg The outcome message (e.g., "OK", "FAIL").
 */
static void wsOtaEnd(bool ok, const String &msg)
{
  wsBroadcastOta([&](JsonDocument &d)
                 { d["event"]="end"; d["ok"]=ok; d["message"]=msg; });
}

/**
 * @brief Returns the `Update` command corresponding to the chosen target.
 *
 * @param fsTarget True if the target is the filesystem partition.
 * @return `U_SPIFFS` or `U_FLASH` depending on the target.
 */
static int updateCmdFromTarget(bool fsTarget) { return fsTarget ? U_SPIFFS : U_FLASH; }

/**
 * @brief Gets the partition designated for the filesystem (SPIFFS or LittleFS).
 *
 * @param[out] out Pointer to the found partition.
 * @param[out] maxSize The maximum size of the partition.
 * @retval true Partition found successfully.
 * @retval false No valid partition found.
 */
static bool getFsPartition(const esp_partition_t **out, size_t *maxSize)
{
#ifdef CONFIG_PARTITION_USE_SPIFFS
  const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
#else
  const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, NULL);
  if (!part)
  {
    part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
  }
#endif

  if (part)
  {
    *out = part;
    *maxSize = part->size;
    return true;
  }

  return false;
}

/**
 * @brief Gets the destination partition for the update (firmware or FS).
 *
 * @param isFs True for the filesystem partition, false for firmware.
 * @param[out] out Pointer to the selected partition.
 * @param[out] maxSize The maximum size of the partition.
 * @retval true Partition found successfully.
 * @retval false No valid partition found.
 */
static bool getTargetPartition(bool isFs, const esp_partition_t **out, size_t *maxSize)
{
  if (isFs)
    return getFsPartition(out, maxSize);
  *out = esp_ota_get_next_update_partition(NULL);
  if (!*out)
    return false;
  *maxSize = (*out)->size;
  return true;
}

/**
 * @brief Mounts the `/update` endpoint for multipart OTA uploads.
 *
 * This function configures the `/update` endpoint to handle file uploads for
 * OTA updates. It manages the entire process from initialization to finalization.
 * - **Handles:**
 * - Partition initialization and size checks.
 * - MD5 validation if the header is present.
 * - Progressive data writing.
 * - WebSocket notifications for status (`start`, `progress`, `end`, `reject`).
 * - Schedules a reboot upon successful completion.
 */
void mountUpdateMultipart()
{
  server.on("/update", HTTP_POST,
            // Success/Failure Handler
            [](AsyncWebServerRequest *request)
            {
            bool success = !Update.hasError();
            request->send(success ? 200 : 500, "text/plain", success ? "OK" : "FAIL");
            
            wsOtaEnd(success, success ? "OK" : "FAIL");
            
            if (success) {
                scheduleReboot(OTA_REQUEST_RESET);
            } },
            // Upload Handler
            [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            {
            static bool began = false;
            static size_t maxSize = 0;
            static const esp_partition_t *part = nullptr;

            if (index == 0) {
                // Initial setup for the update process
                bool isFs = request->hasHeader("X-Update-Target") && 
                            request->getHeader("X-Update-Target")->value().equalsIgnoreCase("fs");

                if (!getTargetPartition(isFs, &part, &maxSize)) {
                    wsOtaReject("Partition not found");
                    Update.abort();
                    return;
                }

                if (request->hasHeader("X-Content-MD5")) {
                    Update.setMD5(request->getHeader("X-Content-MD5")->value().c_str());
                }

                wsOtaStart(isFs, request->contentLength(), maxSize, part->label);

                if (!Update.begin(maxSize, updateCmdFromTarget(isFs))) {
                    wsOtaReject("Update.begin failed");
                    Update.printError(Serial);
                    return;
                }
                began = true;
                written = 0; // Reset della variabile per ogni nuovo upload
            }

            if (began && len > 0) {
                // Write data to the partition
                if (written + len > maxSize) {
                    wsOtaReject("Stream exceeds partition size");
                    Update.abort();
                    return;
                }
                
                size_t w = Update.write(data, len);
                if (w != len) {
                    Update.printError(Serial);
                }
                written += w;

                // Update progress using a rate limit
                uint32_t now = millis();
                if (now - lastProgMs >= 150) {
                    lastProgMs = now;
                    wsOtaProgress(written, request->contentLength() ? request->contentLength() : maxSize);
                }
            }
            
            if (final && began) {
                // Finalize the update
                bool updateSuccess = Update.end(true);
                if (!updateSuccess) {
                    Update.printError(Serial);
                }
                
                wsOtaProgress(written, request->contentLength() ? request->contentLength() : maxSize);
                
                began = false;
            } });
}

/**
 * @brief Mounts the `/ota` endpoint for octet-stream OTA uploads.
 *
 * This function configures the `/ota` endpoint to handle direct binary stream
 * uploads for OTA updates.
 * - **Handles:**
 * - Partition initialization and size checks.
 * - MD5 validation if the header is present.
 * - Progressive data writing.
 * - WebSocket notifications for status (`start`, `progress`, `end`, `reject`).
 * - Schedules a reboot upon successful completion.
 */
void mountUpdateOctet()
{
  server.on("/ota", HTTP_POST,
            // Success/Failure Handler (questo handler è vuoto nel codice originale, e la logica di fine è nel caricamento)
            [](AsyncWebServerRequest *request) {}, nullptr,
            // Upload Handler
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
            static bool began = false;
            static size_t maxSize = 0;
            static const esp_partition_t* part = nullptr;

            if (index == 0) {
                // Initial setup for the update process
                bool isFs = request->hasHeader("X-Update-Target") && 
                            request->getHeader("X-Update-Target")->value().equalsIgnoreCase("fs");

                written = 0; // Reset della variabile per ogni nuovo upload

                if (!getTargetPartition(isFs, &part, &maxSize)) {
                    wsOtaReject("Partition not found");
                    request->send(500, "text/plain", "Partition not found"); 
                    return;
                }
                
                if (total && total > maxSize) {
                    wsOtaReject("Image too big for partition");
                    request->send(400, "text/plain", "Too big"); 
                    return;
                }
                
                if (request->hasHeader("X-Content-MD5")) {
                    Update.setMD5(request->getHeader("X-Content-MD5")->value().c_str());
                }

                wsOtaStart(isFs, total, maxSize, part->label);

                if (!Update.begin(maxSize, updateCmdFromTarget(isFs))) {
                    wsOtaReject("Update.begin failed"); 
                    Update.printError(Serial);
                    request->send(500, "text/plain", "FAIL begin"); 
                    return;
                }
                began = true;
            }

            if (began && len > 0) {
                // Write data to the partition
                if (written + len > maxSize) {
                    wsOtaReject("Stream exceeds partition"); 
                    Update.abort();
                    request->send(400, "text/plain", "Too big"); 
                    return;
                }
                
                size_t w = Update.write(data, len);
                if (w != len) {
                    Update.printError(Serial);
                }
                written += w;

                // Update progress using a rate limit
                uint32_t now = millis();
                if (now - lastProgMs >= 150) {
                    lastProgMs = now;
                    wsOtaProgress(written, total ? total : maxSize);
                }
            }

            if (began && (index + len == total)) {
                // Finalize the update
                bool ok = Update.end(true);
                
                if (!ok) {
                    Update.printError(Serial);
                    request->send(500, "text/plain", "FAIL end");
                    wsOtaEnd(false, "FAIL");
                } else {
                    wsOtaProgress(written, total ? total : maxSize);
                    request->send(200, "text/plain", "OK");
                    wsOtaEnd(true, "OK");
                    scheduleReboot(OTA_REQUEST_RESET);
                }
                began = false;
            } });
}