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
 * @file websocket.h
 * @brief Header for WebSocket functionality management.
 *
 * This file defines the public interface for the WebSocket module.
 * It includes the declaration for the function that initializes the
 * WebSocket server and includes the command map.
 */
#pragma once
#include <map>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "net.h"
#include "motors.h"
#include "telemetry.h"
#include "functionkeys.h" 
#include "display.h"
#include <FifoStringDyn.h>


/**
 * @brief Mounts the WebSocket server onto the application.
 *
 * This function initializes and configures the WebSocket server by associating
 * the event handler (`onWsEvent`) and adding it to the main web server.
 * It serves as the entry point for activating WebSocket communication.
 */
void mountWebSocket();

/**
 * @brief A periodic function for the websocket.
 *
 * A periodic function for the websocket
 */
void websocketTick(void);

/**
 * @brief Check if there are any connected WebSocket clients
 * @return false if there are no connected clients, true otherwise.
 */
bool websocketAreClients(void);

/**
 * @brief Secure load queue message for sending via web server .
 *
 * This function queues messages to be sent asynchronously
 * but in a controlled manner through the web server.
 * @param msg The message push.
 * @return The boolean value of correct push.
 */
bool websocketAsyncMsg(const String &msg);