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
 * @file display.h
 * @brief This header file defines the public API for the display control module.
 *
 * It provides function declarations for initializing the display, managing
 * image uploads, and controlling both static and automatically scrolling
 * text content.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "all_define.h"
#include "utility.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>

/**
 * @defgroup DisplayConfig Display Configuration
 * @{
 */

/**
 * @brief I2C address of the SH1106G display.
 */
#define DISPLAY_I2C_ADD 0x3C
/**
 * @brief Width of the OLED display in pixels.
 */
#define DISPLAY_WIDTH 128
/**
 * @brief Height of the OLED display in pixels.
 */
#define DISPLAY_HEIGHT 64
/**
 * @brief OLED reset pin. Set to -1 as the display is not controlled by a dedicated reset pin in this setup.
 */
#define DISPLAY_OLED_RESET -1

/**
 * @brief Display rotation.
 */
#define DISPLAY_ROTATION 1

/**
 * @brief Calculated size of the image buffer in bytes.
 * @details This is based on the display resolution (128x64) and the fact that a monochrome display uses 1 bit per pixel (128 * 64 / 8).
 */
#define DISPLAY_IMG_SIZE ((DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8)
/**
 * @brief Base character width in pixels for font size 1.
 */
#define DISPLAY_BASE_CHAR_W 6
/**
 * @brief Base character height in pixels for font size 1.
 */
#define DISPLAY_BASE_CHAR_H 8
/**
 * @brief Maximum number of text lines that can be stored in the internal buffer.
 */
#define DISPLAY_MAX_LINES 16
/**
 * @brief Default font size for text display
 */
#define DISPLAY_DEFAULT_TEXT_SIZE 1
/**
 * @brief Default delay in milliseconds between page scrolls.
 */
#define DISPLAY_DELAY_SCROLL 1000

/**
 * @brief Default number of pages for the start text content.
 */
#define DISPLAY_DEFAULT_PAGES 1

/**
 * @brief Scroll pages
 */
#define DISPLAY_SCROLL_MODE_NONE 0

/**
 * @brief Scroll pages
 */
#define DISPLAY_SCROLL_MODE_PAGES 1

/**
 * @brief Scroll line.
 */
#define DISPLAY_SCROLL_MODE_LINES 2

/**
 * @brief Scroll line single step.
 */
#define DISPLAY_SCROLL_PIXEL 1

/**
 * @brief Scroll line pause before step.
 */
#define DISPLAY_SCROLL_PAUSE 300

/** @} */ // end of DisplayConfig

typedef struct sImageParam
{
  /// @brief Image buffer used for storing bitmap data before drawing.
  uint8_t dImgBuf[DISPLAY_IMG_SIZE]; //{};
  /// @brief Expected size of the image data to be received.
  size_t dExpectedSize;
  /// @brief  Size of the image data currently received.
  size_t dReceivedSize;
} ImageParam;

typedef struct sTextParam
{
  /// @brief Global buffer for storing text lines.
  String dLines[DISPLAY_MAX_LINES];
  /// @brief  The number of valid lines currently stored in `dLines`.
  uint8_t dLinesCount;
  /// @brief  The current font size for text display. Default is 1.
  uint8_t dTextSize;
  /// @brief Flag to enable or disable automatic text scrolling.
  bool dAutoScrollOn;
  /// @brief Flag to determine if the text rendering is inverted (white text on a black background).
  bool dInv;
  /// @brief  Flag to determine if text should be truncated to fit the screen width.
  bool dTrunc;
  /// @brief  The delay in milliseconds between page switches during auto-scrolling.
  uint16_t dDelayMs;
  /// @brief  Flag to determine if the auto-scroll should loop back to the first page.
  bool dLoop;
  /// @brief  Total number of pages for the current text content.
  uint8_t dTotalPages;
  /// @brief  The index of the currently displayed page.
  uint8_t dCurrentPage;
  /// @brief  Timestamp of the last page switch, used for managing scroll timing.
  uint32_t dLastSwitchMs;

  uint8_t dMode;       // nonen o pages o lines
  int16_t dYOffsetPx;  // offset verticale in pixel (0..lineH-1)
  uint8_t dTopLineIdx; // indice della riga che sta in alto a schermo
  uint8_t dSpeedPx;    // pixel per step (>=1)
  uint16_t dPauseMs;   // pausa quando si allinea una nuova riga
  bool dPendingPause;  // pausa in corso al bordo-riga?

} TextParam;

/// @brief temporary structure for display parameterization
typedef struct sTmpParam
{

  bool loaded;
  uint8_t mode;
  const String *arr;
  size_t n;
  uint8_t size;
  bool invert;
  bool truncate;
  uint16_t delayMs;
  bool loop;
} TmpParam;

typedef struct sDisplayParam
{
  /// @brief  Display finfit.
  uint8_t dFindiIt;
  /// @brief  Width of the display.
  uint16_t dWidth;
  /// @brief  Height of the display.
  uint16_t dHeigth;
  /// @brief  Reset pin configuration.
  int8_t dOldReset;
  /// @brief  I2C address of the display.
  uint8_t dI2cAdd;
  /// @brief  Size of the image buffer.
  size_t dImgSize;
  /// @brief  Base character width.
  uint8_t dBaseCharW;
  /// @brief  Base character height.
  uint8_t dBaseCharH;
  /// @brief  Maximum number of lines.
  uint8_t dMaxLine;

  TextParam tParam;
  ImageParam iParam;
  TmpParam tmp;

} DisplayParam;

extern DisplayParam DispParam;

/**
 * @brief Initializes the display hardware.
 * @return `true` if the display is successfully initialized and can communicate, `false` otherwise.
 */
bool displayBegin(void);

/**
 * @brief Display module is active.
 *
 * @return `true` if display module is successful, `false` otherwise.
 */
bool displayEnable(void);

/**
 * @brief Clears the display buffer.
 * @param show If `true`, the display is updated immediately to show a blank screen.
 */
void displayClear(bool show);

/*-- Image Handling --*/

/**
 * @defgroup ImageAPI Image Handling Functions
 * @{
 */

/**
 * @brief Renders a full image to the display from a pre-loaded byte array.
 * @param data A pointer to the image data.
 * @param len The size of the image data, which must match the display buffer size.
 */
void displayImage(const uint8_t *data, size_t len);

/**
 * @brief Prepares the internal buffer to receive a new image in chunks.
 * @param totalSize The total expected size of the image data.
 */
void displayStartImageUpload(size_t totalSize);

/**
 * @brief Appends a chunk of image data to the internal buffer.
 * @param data A pointer to the data chunk.
 * @param len The length of the data chunk.
 * @param index The starting position within the image buffer for this chunk.
 * @param total The total expected size of the image.
 */
void displayAppendImageChunk(const uint8_t *data, size_t len, size_t index, size_t total);

/**
 * @brief Draws the content of the image buffer to the display.
 */
void displayDrawImageBuffer(void);

/**
 * @brief Manages the reception and display of an image uploaded in chunks from a server.
 * @param filename The name of the file being transferred.
 * @param index The starting index of the current chunk.
 * @param data A pointer to the data chunk.
 * @param len The length of the data chunk.
 * @param final A flag indicating if this is the last chunk of the image.
 * @return `true` if the image is fully received and displayed, `false` otherwise.
 */
bool displayLoadImageFromServer(const String &filename, size_t index, uint8_t *data, size_t len, bool final);

/**
 * @brief Loading an image from a array.
 *
 * @param data A pointer to the data .
 * @param index The starting index of the current data.
 * @param len The length of the data.
 * @return `true` on success, `false` otherwise.
 */
bool displayLoadImage(uint8_t *data, size_t index, size_t len);

/** @} */ // end of ImageAPI

/*-- Static Text Handling --*/

/**
 * @defgroup TextAPI Text Handling Functions
 * @{
 */

/**
 * @brief Sets the font size for text rendering.
 * @param size The desired font size.
 */
void displaySetTextSize(uint8_t size);

/**
 * @brief Sets the text content for a specific line in the internal buffer.
 * @param idx The line index to update (0-indexed).
 * @param text The string content for the line.
 */
void displaySetLine(uint8_t idx, const String &text);

/**
 * @brief Populates the internal text buffer with an array of strings.
 * @param arr A pointer to an array of `String` objects.
 * @param n The number of strings in the array.
 */
void displaySetLines(const String *arr, size_t n);

/**
 * @brief Clears all text lines from the internal buffer.
 */
void displayClearLines(void);

/**
 * @brief Calculates the maximum number of text lines that can fit on the screen with the current font size.
 * @return The number of visible lines.
 */
uint8_t displayGetMaxVisibleLines(void);

/**
 * @brief Calculates the maximum number of characters that can fit on a single line.
 * @return The number of characters per line.
 */
uint8_t displayGetMaxColsPerLine(void);

/**
 * @brief Renders the text from the internal buffer as a single, static page.
 * @param invert If `true`, the text colors will be inverted.
 * @param truncate If `true`, lines that are too long will be truncated.
 */
void displayRenderTextLines(bool invert, bool truncate);

/**
 * @brief Adds a new line to the end, moving existing lines up.
 * @details Implements a FIFO (First-In, First-Out) logic: the oldest line
 * ​​is deleted and the new line is added at the end.
 * @param text New string to add.
 */
void displayPushLine(const String &text);

/** @} */ // end of TextAPI

/*-- Scrolling and Pagination --*/

/**
 * @defgroup ScrollAPI Scrolling and Pagination Functions
 * @{
 */

/**
 * @brief Load parameter for the automatic scrolling (pagination) of text lines.
 *
 * @details This function sets up the text buffer, font size, and scrolling parameters.
 * It calculates the total number of pages and starts the scroll timer.
 *
 * @param mode Scroll display mode none/pages/lines.
 * @param arr A pointer to the array of strings to display.
 * @param n The number of strings in the array.
 * @param size The font size for the text.
 * @param invert If `true`, the display colors are inverted.
 * @param truncate If `true`, long lines are truncated.
 * @param delayMs The delay in milliseconds between page switches.
 * @param loop If `true`, scrolling loops back to the first page after reaching the last.
 */
void displayLoadAutoScroll(uint8_t mode, const String *arr, size_t n, uint8_t size, bool invert, bool truncate, uint16_t delayMs, bool loop);

/**
 * @brief Starts the automatic scrolling (pagination) of text lines.
 *
 * @details This function sets up the text buffer, font size, and scrolling parameters.
 * It calculates the total number of pages and starts the scroll timer.
 *
 * @param mode Scroll display mode none/pages/lines.
 * @param arr A pointer to the array of strings to display.
 * @param n The number of strings in the array.
 * @param size The font size for the text.
 * @param invert If `true`, the display colors are inverted.
 * @param truncate If `true`, long lines are truncated.
 * @param delayMs The delay in milliseconds between page switches.
 * @param loop If `true`, scrolling loops back to the first page after reaching the last.
 */
void displayStartAutoScroll(uint8_t mode, const String *arr, size_t n, uint8_t size, bool invert, bool truncate, uint16_t delayMs, bool loop);

/**
 * @brief Renders a specific page of text to the display.
 * @param pageIndex The 0-indexed page number to render.
 */
void displayRenderPage(uint8_t pageIndex);

/**
 * @brief Renders a specific line of text to the display.
 */
void displayRenderScrolled(void);

/**
 * @brief Stops the automatic scrolling process.
 */
void displayStopAutoScroll(void);

/**
 * @brief A non-blocking function to be called periodically in the main loop to manage auto-scrolling.
 */
void displaytick(void);

/** @} */
