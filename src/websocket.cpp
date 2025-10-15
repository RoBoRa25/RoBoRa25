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
 * @file websocket.cpp
 * @brief Implementation of the WebSocket server for command handling.
 *
 * This file contains the complete implementation of the WebSocket server,
 * managing connection, sending, and receiving messages in JSON format. It
 * defines command handlers and the logic for processing messages.
 */

#include "websocket.h"

/**
 * @typedef WsCommandHandler
 * @brief Function pointer for WebSocket command handling.
 *
 * Defines the type for functions that handle commands received via WebSocket.
 * Each handler takes a pointer to the WebSocket client and a JSON document
 * containing the message payload as parameters.
 * @param client Pointer to the WebSocket client that sent the message.
 * @param doc Reference to the JSON document containing the message payload.
 */
typedef void (*WsCommandHandler)(AsyncWebSocketClient *, JsonDocument &);

/*-- Function declarations (forward declaration) --*/
static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
static void ws_connect_hello(AsyncWebSocketClient *client);
static void handleWsMessage(AsyncWebSocketClient *client, const char *payload, size_t len);
static void handleWsBinary(AsyncWebSocketClient *client, const uint8_t *payload, size_t len);
static void WsSendJson(AsyncWebSocketClient *client, const JsonDocument &doc);
static void ws_cmd_error(AsyncWebSocketClient *client, String Errortype);

/*-- Command handler declarations --*/
static void ws_cmd_hello(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_reboot(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_config_req(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_config_rd(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_config_wr(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_sendInfo(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_move(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_function(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_reset_memory(AsyncWebSocketClient *client, JsonDocument &doc);
static void ws_cmd_sendString(AsyncWebSocketClient *client, JsonDocument &doc);

/**
 * @brief FifoStringDyn object instance for Fifo String.
 */
FifoStringDyn Fsd;

/**
 * @brief Secure load queue message for sending via web server .
 *
 * This function queues messages to be sent asynchronously
 * but in a controlled manner through the web server.
 * @param msg The message push.
 * @return The boolean value of correct push.
 */
bool websocketAsyncMsg(const String &msg)
{
  uint8_t idClient;
  bool RET = false;
  if (websocketAreClients())
    RET = Fsd.push(msg);
  return RET;
}

/**
 * @brief Secure pop queue message sending via web server .
 *
 * This function sends messages to the web server is present
 * in the queue
 * @param AreClient client presence
 * @return The current number of elements in the queue
 */
size_t websocketSendAsyncMsg(bool AreClient)
{
  String s;
  if (AreClient) // client
  {
    if (!Fsd.isEmpty()) // message
    {
      if (Fsd.pop(s)) // extract mesg
      {
        ws.textAll(s); // send
      }
    }
  }
  return Fsd.size();
}

/*-- Helper for safe parameter extraction --*/
/**
 * @brief Safely retrieves a boolean from a JsonVariant.
 *
 * This function attempts to get a boolean value from the given JsonVariant.
 * It supports conversions from boolean, integer, and string types.
 * - An integer value of 0 is considered false, any other value is true.
 * - A string is first converted to an integer, then treated as above.
 *
 * @param v The JsonVariant to read from.
 * @param def The default value to return if the type is not a valid boolean, integer, or string.
 * @return The boolean value from the variant, or the default value if the type is not recognized.
 */
bool ws_getBool(const JsonVariant &v, bool def)
{
  if (v.is<bool>())
    return v.as<bool>();
  if (v.is<int>())
    return v.as<int>() != 0;
  if (v.is<const char *>())
    return String(v.as<const char *>()).toInt() != 0;
  return def;
}

/**
 * @brief Safely retrieves an 8-bit unsigned integer (uint8_t) from a JsonVariant.
 *
 * This function handles potential negative values and overflows by clamping the
 * value to the valid range of uint8_t (0-255).
 *
 * @param v The JsonVariant to read from.
 * @param def The default value to return if the variant is null.
 * @return The 8-bit unsigned integer value, clamped to [0, 255], or the default value.
 */
uint8_t ws_getU8(const JsonVariant &v, uint8_t def)
{
  if (!v.isNull())
  {
    int x = v.as<int>();
    if (x < 0)
      x = 0;
    if (x > 255)
      x = 255;
    return (uint8_t)x;
  }
  return def;
}

/**
 * @brief Safely retrieves a 16-bit unsigned integer (uint16_t) from a JsonVariant.
 *
 * This function handles potential negative values and overflows by clamping the
 * value to the valid range of uint16_t (0-65535).
 *
 * @param v The JsonVariant to read from.
 * @param def The default value to return if the variant is null.
 * @return The 16-bit unsigned integer value, clamped to [0, 65535], or the default value.
 */
uint16_t ws_getU16(const JsonVariant &v, uint16_t def)
{
  if (!v.isNull())
  {
    long x = v.as<long>();
    if (x < 0)
      x = 0;
    if (x > 65535)
      x = 65535;
    return (uint16_t)x;
  }
  return def;
}

/* Simple connection pool for handling WebSocket messages */
/**
 * @struct WsAcc
 * @brief Represents a WebSocket connection accumulator.
 *
 * This structure is used to accumulate data frames for a single WebSocket
 * message, which may be fragmented across multiple frames. It stores the
 * state and buffer for a specific client connection.
 */
typedef struct sWsAcc
{
  uint32_t id = 0;          ///< @brief The unique identifier for the WebSocket connection.
  bool inUse = false;       ///< @brief Flag to indicate if this accumulator slot is currently in use.
  uint8_t firstOpcode = 0;  ///< @brief The opcode of the first frame (TEXT or BINARY) of the message.
  size_t expectedLen = 0;   ///< @brief The total expected length of the complete message.
  std::vector<uint8_t> buf; ///< @brief A byte-precise buffer to accumulate the message data.
} WsAcc;

/**
 * @var static WsAcc s_acc[WS_MAX_CLIENTS]
 * @brief A static array of WsAcc structs to manage a pool of WebSocket accumulators.
 *
 * This array acts as a fixed-size pool to handle multiple simultaneous
 * WebSocket client connections, limited by the WS_MAX_CLIENTS constant.
 */
static WsAcc s_acc[WS_MAX_CLIENTS];

/**
 * @brief Map of WebSocket commands.
 *
 * A static map that associates a command string (key) with a function pointer
 * (value). This approach allows for efficient dispatching of received commands.
 */
static std::map<String, WsCommandHandler> ws_commands = {
    {"hello_robora", ws_cmd_hello},
    {"reboot", ws_cmd_reboot},
    {"config_req", ws_cmd_config_req},
    {"config_rd", ws_cmd_config_rd},
    {"config_wr", ws_cmd_config_wr},
    {"info_req", ws_cmd_sendInfo},
    {"move", ws_cmd_move},
    {"function", ws_cmd_function},
    {"reset_memory", ws_cmd_reset_memory},
    {"displaymsg", ws_cmd_sendString}};

/**
 * @brief Sends a JSON document to a client or broadcasts it.
 *
 * Serializes a JSON document into a string and sends it to the specified client.
 * If the client pointer is null, the message is broadcast to all connected clients.
 *
 * @param client Pointer to the destination client. If null, the message is broadcast.
 * @param doc Reference to the JSON document to be sent.
 */
static void WsSendJson(AsyncWebSocketClient *client, const JsonDocument &doc)
{
  String s;
  if (serializeJson(doc, s) == 0)
    return;

  if (client) // single client
    client->text(s);
  else // brodcast
    ws.textAll(s);
}

/**
 * @brief Handler for the "hello" command.
 *
 * Responds to the client with a `hello_ack` message containing application
 * version information.
 *
 * @param client Pointer to the client.
 */
static void ws_connect_hello(AsyncWebSocketClient *client)
{
  JsonDocument ack;
  ack["CMD"] = "hello_webui";
  ack["server"] = CONNECTION_HOSTNAME;
  ack["ver"] = VERSIONE_APP;
  WsSendJson(client, ack);
}

/**
 * @brief Handles a received WebSocket message TEXT.
 *
 * This function deserializes the JSON message payload, extracts the command,
 * and looks it up in the `ws_commands` map. If the command is found, it calls
 * the associated handler function. Otherwise, it sends an error message.
 *
 * @param client Pointer to the client that sent the message.
 * @param payload Pointer to the message payload (JSON string).
 * @param len Length of the payload.
 */
static void handleWsMessage(AsyncWebSocketClient *client, const char *payload, size_t len)
{
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  if (err)
  {
    JsonDocument error_doc;
    error_doc["CMD"] = "error";
    error_doc["msg"] = "invalid json payload" + String(err.c_str());
    WsSendJson(client, error_doc);
    return;
  }

  const char *cmd = doc["CMD"] | "";

  auto it = ws_commands.find(cmd);
  if (it != ws_commands.end())
  {
    it->second(client, doc);
  }
  else
  {
    ws_cmd_error(client, "unknown command");
  }
}

/**
 * @brief Handles a received WebSocket message BINARY.
 *
 * @param client Pointer to the client that sent the message.
 * @param payload Pointer to the message payload (*******).
 * @param len Length of the payload.
 */
static void handleWsBinary(AsyncWebSocketClient *client, const uint8_t *payload, size_t len)
{
  // Not Use
}

/**
 * @brief Handler for the "hello" command.
 *
 * Responds to the client with a `hello_ack` message containing application
 * version information.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document.
 */
static void ws_cmd_hello(AsyncWebSocketClient *client, JsonDocument &doc)
{
  JsonDocument ack;
  ack["CMD"] = "hello_webui";
  ack["server"] = CONNECTION_HOSTNAME;
  ack["ver"] = VERSIONE_APP;
  WsSendJson(client, ack);
}

/**
 * @brief Handler for the "reboot" command.
 *
 * Sends a confirmation response and schedules the device to reboot.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document.
 */
static void ws_cmd_reboot(AsyncWebSocketClient *client, JsonDocument &doc)
{
  client->text("{\"CMD\":\"ack\",\"msg\":\"rebooting\"}");
  scheduleReboot(WS_REQUEST_RESET);
}

/**
 * @brief Handler for the "config_req" command.
 *
 * Sends the complete list of available configuration parameters to the client.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document (unused in this case).
 */
static void ws_cmd_config_req(AsyncWebSocketClient *client, JsonDocument &doc)
{
  String s;
  s = configGetListParameter();
  if (client) // single client
    client->text(s);
  else // brodcast
    ws.textAll(s);
}

/**
 * @brief Handler for the "config_rd" (read configuration) command.
 *
 * Reads the configuration parameters specified in the JSON message and sends
 * the values back to the client.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document containing the keys to read.
 */
static void ws_cmd_config_rd(AsyncWebSocketClient *client, JsonDocument &doc)
{
  for (JsonPair kv : doc.as<JsonObject>())
  {
    const char *k = kv.key().c_str();

    if (configIsParamKey(k))
    {
      String v = configGet(k, "");
      JsonDocument r;
      r["CMD"] = "config_rd";
      r[k] = v;
      WsSendJson(client, r);
    }
  }
}

/**
 * @brief Handler for the "config_wr" (write configuration) command.
 *
 * Writes the configuration parameters specified in the JSON message.
 * After writing, it re-initializes the motors to apply the changes.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document containing the keys and values to write.
 */
static void ws_cmd_config_wr(AsyncWebSocketClient *client, JsonDocument &doc)
{
  for (JsonPair kv : doc.as<JsonObject>())
  {
    const char *k = kv.key().c_str();
    if (configIsParamKey(k))
    {
      const char *v = kv.value().as<const char *>();
      String sv = v ? v : "";
      configPut(k, sv);
      JsonDocument r;
      r["CMD"] = "config_wr";
      r[k] = sv;
      WsSendJson(client, r);
    }
  }
  motorsReload();    // motorsInit();
  telemetryReload(); // telemetryInit();
}

/**
 * @brief Handler for the "info_req" command.
 *
 * Sends a JSON document to the client containing system status information,
 * such as the IP address, Wi-Fi signal strength, uptime, and available heap memory.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document (unused in this case).
 */
static void ws_cmd_sendInfo(AsyncWebSocketClient *client, JsonDocument &doc)
{
  JsonDocument Infos;
  Infos["CMD"] = "info";
  Infos["info1"] = "Versione RoBoRa: " + String(VERSIONE_APP);
  Infos["info2"] = "Chip ID:" + String(ESP.getEfuseMac()) + " Ver. Chip:" + String(ESP.getChipRevision()) + " Core:" + String(ESP.getChipCores()) + " " + String(ESP.getCpuFreqMHz()) + "Mhz IDF:" + ESP.getSdkVersion();
  Infos["info3"] = WiFi.localIP().toString() + " " + WiFi.macAddress() + " " + String(WiFi.RSSI()) + " dBm  " + String(millis() / 1000) + " s uptime";
  Infos["info4"] = "Flash Size:" + String(ESP.getFlashChipSize() / 1024) + " KB Free Space:" + String(ESP.getFreeSketchSpace() / 1024) + " KB";
#ifdef CONFIG_PARTITION_USE_SPIFFS
  Infos["info5"] = "Memory: " + String(ESP.getFreeHeap() / 1024) + " KB heap + SPIFFS: " + String(SPIFFS.usedBytes() / 1024) + "/" + String(SPIFFS.totalBytes() / 1024) + " KB FS";
#else
  Infos["info5"] = "Memory: " + String(ESP.getFreeHeap() / 1024) + " KB heap + SPIFFS: " + String(LittleFS.usedBytes() / 1024) + "/" + String(LittleFS.totalBytes() / 1024) + " KB FS";
#endif
  Infos["info6"] = "SPARE";
  Infos["info7"] = "SPARE";
  Infos["info8"] = "SPARE";
  WsSendJson(client, Infos);
}

/**
 * @brief Handler for the "move" command.
 *
 * Receives movement coordinates (x, y) from the JSON message, constrains them
 * to the range [-127, 127], and delegates them to the motors module for execution.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document containing the `x` and `y` coordinates.
 */
static void ws_cmd_move(AsyncWebSocketClient *client, JsonDocument &doc)
{
  int x = atoi((doc["x"] | "0"));
  int y = atoi((doc["y"] | "0"));
  x = constrain(x, -127, 127);
  y = constrain(y, -127, 127);
  motorsApply(y, x);
  JsonDocument r;
  r["CMD"] = "move";
  r["status"] = "OK";
  WsSendJson(client, r);
}

/**
 * @brief Handler for the "function" command.
 *
 * Receives the state (on/off) for function keys (e.g., "FN1", "FN2").
 * The commands are then delegated to the function keys module for action.
 *
 * @param client Pointer to the client.
 * @param doc Reference to the JSON document containing the function keys and their states.
 */
static void ws_cmd_function(AsyncWebSocketClient *client, JsonDocument &doc)
{
  for (JsonPair kv : doc.as<JsonObject>())
  {
    const char *k = kv.key().c_str();
    if (strncmp(k, "FN", 2) == 0)
    {
      int idx = atoi(k + 2);
      const char *v = kv.value().as<const char *>();
      bool on = (v && strcasecmp(v, "on") == 0);
      fnSet(idx, on); // <--- delega al modulo tasti
    }
  }
  JsonDocument r;
  r["CMD"] = "function";
  r["status"] = "OK";
  WsSendJson(client, r);
}

/**
 * @brief Handler for reset memory command.
 *
 * This is called to reset memory to default
 *
 * @param client Pointer to the client that sent the unknown command.
 * @param doc Reference to the original command's JSON document.
 */
static void ws_cmd_reset_memory(AsyncWebSocketClient *client, JsonDocument &doc)
{
  configSaveAllDefaults();
  JsonDocument r;
  r["CMD"] = "reset_memory";
  r["status"] = "OK";
  WsSendJson(client, r);
}

/**
 * @brief Handler for write string into display.
 *
 * This is called to write message into dispaly
 *
 * @param client Pointer to the client that sent the unknown command.
 * @param doc Reference to the original command's JSON document.
 */
static void ws_cmd_sendString(AsyncWebSocketClient *client, JsonDocument &doc)
{
  uint8_t fontsize = ws_getU8(doc["size"], 1);
  bool invert = ws_getBool(doc["invert"], false);
  bool truncate = ws_getBool(doc["truncate"], false);
  uint8_t scroll = ws_getU8(doc["scroll"], 0);
  uint16_t delayMs = ws_getU16(doc["delay"], 1500);
  bool loop = ws_getBool(doc["loop"], false);

  static String buf[DISPLAY_MAX_LINES];
  uint8_t Stringrecived = 0;
  if (doc["strings"].is<JsonArray>())
  {
    JsonArray arr = doc["strings"].as<JsonArray>();
    for (JsonVariant v : arr)
    {
      if (v.is<const char *>())
        buf[Stringrecived++] = String(v.as<const char *>());
    }
  }
  displayLoadAutoScroll(scroll, buf, Stringrecived, fontsize, invert, truncate, delayMs, loop);
  JsonDocument r;
  r["CMD"] = "displaymsg";
  r["status"] = "OK";
  WsSendJson(client, r);
}

/**
 * @brief Handler for the error command.
 *
 * This is called when an unknown command is received. It sends an error message
 * to the client specifying that the command is not recognized.
 *
 * @param client Pointer to the client that sent the unknown command.
 * @param Errortype Reference to type of error.
 */
static void ws_cmd_error(AsyncWebSocketClient *client, String Errortype)
{
  JsonDocument r;
  r["CMD"] = "error";
  r["msg"] = Errortype;
  WsSendJson(client, r);
}

/**
 * @fn static WsAcc* WsGetAcc(uint32_t id)
 * @brief Retrieves an accumulator for a given connection ID, or allocates a new one.
 * @param id The unique identifier of the WebSocket connection.
 * @return A pointer to the WsAcc struct for the specified ID, or a new one if available. Returns nullptr if no slot is free.
 *
 * This function first searches for an existing accumulator with the given ID.
 * If found, it returns a pointer to it. If not, it looks for a free slot in the
 * s_acc array, initializes it with the new ID, and returns a pointer to it.
 * If the pool is full, it returns nullptr.
 */
static WsAcc *WsGetAcc(uint32_t id)
{
  // Check for an existing accumulator
  for (auto &a : s_acc)
    if (a.inUse && a.id == id)
      return &a;
  // Find a free slot
  for (auto &a : s_acc)
    if (!a.inUse)
    {
      a.inUse = true;
      a.id = id;
      a.buf.clear();
      a.expectedLen = 0;
      a.firstOpcode = 0;
      return &a;
    }
  return nullptr; // No free slot
}

/**
 * @fn static void WsResetAcc(WsAcc* a)
 * @brief Resets the state of a specific accumulator.
 * @param a A pointer to the WsAcc struct to be reset.
 *
 * This function clears the accumulator's buffer and resets its state variables.
 * It does not release the slot, so the accumulator remains "in use".
 */
static void WsResetAcc(WsAcc *a)
{
  if (!a)
    return;
  a->buf.clear();
  a->expectedLen = 0;
  a->firstOpcode = 0;
}

/**
 * @fn static void WsReleaseAcc(uint32_t id)
 * @brief Releases the accumulator slot associated with a given connection ID.
 * @param id The unique identifier of the WebSocket connection to release.
 *
 * This function searches for the accumulator with the specified ID and marks
 * it as "not in use," making it available for new connections. It also clears
 * its internal state.
 */
static void WsReleaseAcc(uint32_t id)
{
  for (auto &a : s_acc)
  {
    if (a.inUse && a.id == id)
    {
      a.inUse = false;
      a.buf.clear();
      a.expectedLen = 0;
      a.firstOpcode = 0;
      break;
    }
  }
}

/**
 * @brief WebSocket server event handler.
 *
 * A callback function invoked whenever a WebSocket event occurs (e.g., connection,
 * disconnection, data reception). It manages new clients by sending a welcome
 * message and delegates data handling to `handleWsMessage`.
 *
 * @param server Pointer to the WebSocket server instance.
 * @param client Pointer to the client that triggered the event.
 * @param type The event type (e.g., `WS_EVT_CONNECT`, `WS_EVT_DATA`).
 * @param arg Optional argument, in this case the frame information.
 * @param data Pointer to the received data.
 * @param len Length of the data.
 */
static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    if (WsAcc *acc = WsGetAcc(client->id()))
      WsResetAcc(acc);
    ws_connect_hello(client);
    return;
  }

  if (type == WS_EVT_DISCONNECT)
  {
    WsReleaseAcc(client->id());
    return;
  }

  if (type != WS_EVT_DATA)
    return;

  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  WsAcc *acc = WsGetAcc(client->id());
  if (!acc)
  {
    ws_cmd_error(client, "too many ws clients");
    return;
  }

  // Primo chunk del messaggio
  if (info->index == 0)
  {
    WsResetAcc(acc);
    acc->firstOpcode = info->opcode; // TEXT o BINARY (i frame successivi avranno opcode 0 = continuation)
    acc->expectedLen = info->len;    // dimensione totale annunciata

    if (acc->expectedLen > WS_MAX_PAYLOAD)
    {
      ws_cmd_error(client, "payload too large");
      WsResetAcc(acc);
      return;
    }
    acc->buf.reserve(acc->expectedLen); // riduce riallocazioni
  }

  // Accumula i byte del chunk corrente (non sono null-terminati)
  acc->buf.insert(acc->buf.end(), data, data + len);

  // Se non è l'ultimo, aspetta altri chunk
  if (!info->final)
    return;

  // Dispatch finale in base all'opcode del PRIMO frame
  if (acc->firstOpcode == WS_TEXT)
  {
    // Aggiungo un terminatore per sicurezza e passo al handler
    acc->buf.push_back(0);
    handleWsMessage(client, (const char *)acc->buf.data(), acc->buf.size() - 1);
  }
  else if (acc->firstOpcode == WS_BINARY)
  {
    handleWsBinary(client, acc->buf.data(), acc->buf.size());
  }
  else
  {
    ws_cmd_error(client, "unsupported opcode");
  }

  // Pronto per un nuovo messaggio
  WsResetAcc(acc);
}

/**
 * @brief Mounts the WebSocket server.
 *
 * Configures the WebSocket server by setting the event callback and adding
 * the handler to the main server. This function is called at application startup.
 */
void mountWebSocket()
{
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}

/**
 * @brief A periodic function for the websocket.
 *
 * A periodic function for the websocket
 */
void websocketTick(void)
{
  static bool AreClient;
  ws.cleanupClients();
  AreClient = websocketAreClients();
  websocketSendAsyncMsg(AreClient);
  if(!AreClient)motorsApply(0,0);
}

/**
 * @brief Check if there are any connected WebSocket clients
 * @return false if there are no connected clients, true otherwise.
 */
bool websocketAreClients(void)
{
  // Cerca il primo client connesso.
  for (uint8_t idClient = 0; idClient < DEFAULT_MAX_WS_CLIENTS; idClient++)
  {
    if (ws.hasClient(idClient))
      return true;
  }
  return false;
}