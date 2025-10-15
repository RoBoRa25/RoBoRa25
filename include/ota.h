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
 * @file ota.h
 * @brief Declarations for Over-The-Air (OTA) update functions.
 *
 * This header exposes the main interfaces for mounting HTTP endpoints dedicated to
 * updating the device's firmware or filesystem.
 */
#pragma once
#include <Update.h>
#include "config.h"
#include "websocket.h"

extern "C"
{
#include "esp_ota_ops.h"
#include "esp_partition.h"
}

/**
 * @brief Mounts the HTTP endpoint for OTA updates with multipart uploads.
 *
 * The exposed endpoint is `/update`. It accepts files uploaded via form-data.
 * After completion, it sends the response and, in case of success, schedules a reboot.
 *
 * @note This function uses the ESP-IDF/Arduino `Update` module.
 */
void mountUpdateMultipart(); 

/**
 * @brief Mounts the HTTP endpoint for OTA updates with octet-stream.
 *
 * The exposed endpoint is `/ota`. It accepts a direct binary stream.
 * It internally handles progress, MD5 validation, and reboots upon success.
 */
void mountUpdateOctet(); 

