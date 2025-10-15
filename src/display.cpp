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
 * @file display.cpp
 * @brief This file provides a comprehensive API for controlling an Adafruit SH1106G OLED display.
 *
 * The implementation handles both image rendering and text display, including features like
 * automatic text scrolling and pagination for content that exceeds the display's visible area.
 * It's designed for use in embedded systems, specifically with the Arduino framework.
 */

#include "display.h"

/**
 * @brief DisplayParam object instance for display control.
 */
DisplayParam DispParam;

/**
 * @brief Adafruit_SH1106G object instance for display control.
 */
Adafruit_SH1106G *disPlay;

/**
 * @brief Finalizes and displays the image that has been uploaded to the buffer.
 *
 * @details This function is an alias for `displayDrawImageBuffer()` and is meant to be called after
 * all image data chunks have been appended.
 */
void displayFinalizeImageUpload() { displayDrawImageBuffer(); }

/**
 * @brief Initializes the display module.
 *
 * @return `true` if initialization is successful, `false` otherwise.
 */
bool displayBegin(void)
{

  bool ret = false;
  DispParam.dFindiIt = 0;
  DispParam.dWidth = DISPLAY_WIDTH;
  DispParam.dHeigth = DISPLAY_HEIGHT;
  DispParam.dOldReset = DISPLAY_OLED_RESET;
  DispParam.dI2cAdd = DISPLAY_I2C_ADD;
  DispParam.dImgSize = DISPLAY_IMG_SIZE;
  DispParam.dBaseCharW = DISPLAY_BASE_CHAR_W;
  DispParam.dBaseCharH = DISPLAY_BASE_CHAR_H;
  DispParam.dMaxLine = DISPLAY_MAX_LINES;

  // Image buffer variables
  DispParam.iParam.dImgBuf[DISPLAY_IMG_SIZE] = {};
  DispParam.iParam.dExpectedSize = 0;
  DispParam.iParam.dReceivedSize = 0;

  // Text buffer variables
  DispParam.tParam.dLinesCount = 0;
  DispParam.tParam.dLines[DISPLAY_MAX_LINES];
  DispParam.tParam.dLinesCount = 0;
  DispParam.tParam.dTextSize = DISPLAY_DEFAULT_TEXT_SIZE;
  // Scrolling state variables
  DispParam.tParam.dAutoScrollOn = false;
  DispParam.tParam.dInv = false;
  DispParam.tParam.dTrunc = true;
  DispParam.tParam.dDelayMs = DISPLAY_DELAY_SCROLL;
  DispParam.tParam.dLoop = true;
  DispParam.tParam.dTotalPages = DISPLAY_DEFAULT_PAGES;
  DispParam.tParam.dCurrentPage = 0;
  DispParam.tParam.dLastSwitchMs = 0;

  // scrolling mode
  DispParam.tParam.dMode = DISPLAY_SCROLL_MODE_NONE;
  DispParam.tParam.dYOffsetPx = 0;
  DispParam.tParam.dTopLineIdx = 0;
  DispParam.tParam.dSpeedPx = DISPLAY_SCROLL_PIXEL;
  DispParam.tParam.dPauseMs = DISPLAY_SCROLL_PAUSE;
  DispParam.tParam.dPendingPause = false;

  // check
  if (checkI2CDevice(DispParam.dI2cAdd))
  {
    if (disPlay == nullptr)
      disPlay = new Adafruit_SH1106G(DispParam.dWidth, DispParam.dHeigth, &Wire, DispParam.dOldReset);

    if (disPlay->begin(DispParam.dI2cAdd, true))
    {
      DispParam.dFindiIt = 1;
      // disPlay->setRotation(DISPLAY_ROTATION);
      disPlay->display();
      disPlay->clearDisplay();
      disPlay->display();
      ret = true;
    }
  }

  DEBUG_PRINTF("DISPLAY initialization : %s\n", DispParam.dFindiIt ? "OK" : "KO");

  return ret;
}

/**
 * @brief Display module is active.
 *
 * @return `true` if display module is successful, `false` otherwise.
 */
bool displayEnable(void)
{
  return DispParam.dFindiIt ? true : false;
}

/**
 * @brief Clears the display buffer.
 *
 * @param show If `true`, the `disPlay.display()` command is called to update the screen immediately.
 */
void displayClear(bool show)
{
  disPlay->clearDisplay();
  if (show)
    disPlay->display();
}

/*-- Immagine --*/
/**
 * @brief Displays a full image from a given byte array.
 *
 * @param data A pointer to the image data. The data must be in the correct bitmap format for the display.
 * @param len The length of the image data in bytes. This must match `DISPLAY_IMG_SIZE`.
 */
void displayImage(const uint8_t *data, size_t len)
{
  if (!data || len != DispParam.dImgSize)
    return;
  memcpy(DispParam.iParam.dImgBuf, data, DispParam.dImgSize);
  displayDrawImageBuffer();
}

/**
 * @brief Prepares the system to receive a new image in chunks.
 *
 * @param totalSize The total expected size of the image data in bytes. This function
 * caps the expected size at `dImgSize`.
 */
void displayStartImageUpload(size_t totalSize)
{
  DispParam.iParam.dExpectedSize = totalSize > DispParam.dImgSize ? DispParam.dImgSize : totalSize;
  DispParam.iParam.dReceivedSize = 0;
  memset(DispParam.iParam.dImgBuf, 0x00, DispParam.dImgSize);
}

/**
 * @brief Appends a chunk of image data to the internal buffer.
 *
 * @param data A pointer to the image data chunk.
 * @param len The length of the data chunk.
 * @param index The starting index within the image buffer for this chunk.
 * @param total The total expected size of the image (unused but included for context).
 */
void displayAppendImageChunk(const uint8_t *data, size_t len, size_t index, size_t total)
{
  (void)total;
  if (!data || len == 0 || index >= DispParam.dImgSize)
    return;
  size_t canCopy = (index + len <= DispParam.dImgSize) ? len : (DispParam.dImgSize - index);
  memcpy(DispParam.iParam.dImgBuf + index, data, canCopy);
  size_t endPos = index + canCopy;
  if (endPos > DispParam.iParam.dReceivedSize)
    DispParam.iParam.dReceivedSize = endPos;
}

/**
 * @brief Draws the image stored in the internal buffer to the display.
 *
 * @details This function clears the display, draws the bitmap from `dImgBuf`, and then updates the screen.
 */
void displayDrawImageBuffer(void)
{
  DispParam.tParam.dLoop = false;
  DispParam.tParam.dAutoScrollOn = false;
  disPlay->clearDisplay();
  disPlay->drawBitmap(0, 0, DispParam.iParam.dImgBuf, DispParam.dWidth, DispParam.dHeigth, WHITE);
  disPlay->display();
}

/**
 * @brief Handles the complete process of loading an image from a remote source (e.g., a server).
 *
 * @details This function combines `displayStartImageUpload()` and `displayAppendImageChunk()`
 * to manage chunked image transfers. It automatically calls `displayFinalizeImageUpload()`
 * when the last chunk is received.
 *
 * @param filename The name of the file being transferred (currently unused).
 * @param index The starting index of the current data chunk.
 * @param data A pointer to the data chunk.
 * @param len The length of the data chunk.
 * @param final A flag indicating if this is the last chunk of the image.
 * @return `true` on success (image fully received and displayed), `false` otherwise.
 */
bool displayLoadImageFromServer(const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!DispParam.dFindiIt)
    return false;
  if (!index)
  {
    displayStartImageUpload(DispParam.dImgSize);
  }
  displayAppendImageChunk(data, len, index, DispParam.dImgSize);
  if ((final) && ((index + len == DispParam.dImgSize)))
  {
    displayFinalizeImageUpload();
    DEBUG_PRINTLN("Immagine ricevuta e visualizzata.");
  }
  else
  {
    DEBUG_PRINTLN("Errore: Immagine troppo grande per il buffer.");
    return false;
  }
  return true;
}

/**
 * @brief Loading an image from a array.
 *
 * @param data A pointer to the data .
 * @param index The starting index of the current data.
 * @param len The length of the data.
 * @return `true` on success, `false` otherwise.
 */
bool displayLoadImage(uint8_t *data, size_t index, size_t len)
{
  return displayLoadImageFromServer("FromFile", 0, data, DISPLAY_IMG_SIZE, true);
}

/*-- Single Page Text Display --*/

/**
 * @brief Sets the font size for text rendering.
 *
 * @param size The desired font size. A value of 0 or 1 sets the size to 1.
 */
void displaySetTextSize(uint8_t size)
{
  if (size)
    DispParam.tParam.dTextSize = size;
  else
    DispParam.tParam.dTextSize = 1;
}

/**
 * @brief Sets the text content for a specific line in the internal buffer.
 *
 * @param idx The line index to update (0-indexed).
 * @param text The new string content for the line.
 */
void displaySetLine(uint8_t idx, const String &text)
{
  if (idx >= DispParam.dMaxLine)
    return;
  DispParam.tParam.dLines[idx] = text;
  if (idx + 1 > DispParam.tParam.dLinesCount)
    DispParam.tParam.dLinesCount = idx + 1;
}

/**
 * @brief Populates the internal text buffer from a provided array of strings.
 *
 * @param arr A pointer to an array of `String` objects.
 * @param n The number of strings in the array.
 */
void displaySetLines(const String *arr, size_t n)
{
  if (!arr)
    return;
  size_t m = n > DispParam.dMaxLine ? DispParam.dMaxLine : n;
  for (size_t i = 0; i < m; ++i)
    DispParam.tParam.dLines[i] = arr[i];
  for (size_t i = m; i < DispParam.dMaxLine; ++i)
    DispParam.tParam.dLines[i] = "";
  DispParam.tParam.dLinesCount = m;
}

/**
 * @brief Clears all text lines in the internal buffer.
 */
void displayClearLines(void)
{
  for (uint8_t i = 0; i < DispParam.dMaxLine; ++i)
    DispParam.tParam.dLines[i] = "";
  DispParam.tParam.dLinesCount = 0;
}

/**
 * @brief Calculates the maximum number of text lines that can fit on the screen with the current font size.
 *
 * @return The maximum number of visible lines.
 */
uint8_t displayGetMaxVisibleLines(void)
{
  uint16_t lineH = DispParam.dBaseCharH * DispParam.tParam.dTextSize;
  if (!lineH)
    return 1;
  uint8_t lines = DispParam.dHeigth / lineH;
  return lines == 0 ? 1 : lines;
}

/**
 * @brief Calculates the maximum number of characters (columns) that can fit on a single line.
 *
 * @return The maximum number of characters per line.
 */
uint8_t displayGetMaxColsPerLine(void)
{
  uint16_t charW = DispParam.dBaseCharW * DispParam.tParam.dTextSize;
  if (!charW)
    return 1;
  uint8_t cols = DispParam.dWidth / charW;
  return cols == 0 ? 1 : cols;
}

/**
 * @brief Renders the text from the internal buffer to the display as a single page.
 *
 * @param invert If `true`, the display colors are inverted (black text on white).
 * @param truncate If `true`, lines longer than the display width are truncated.
 */
void displayRenderTextLines(bool invert, bool truncate)
{
  DispParam.tParam.dInv = invert;     // memorizzo anche per eventuale switch pagina manuale
  DispParam.tParam.dTrunc = truncate; // idem
  displayRenderPage(0);               // una sola pagina (righe 0..vis-1)
}

/**
 * @brief Adds a new line to the end, moving existing lines up.
 * @details Implements a FIFO (First-In, First-Out) logic: the oldest line
 * ​​is deleted and the new line is added at the end.
 * @param text New string to add.
 */
void displayPushLine(const String &text)
{
  // 1. Moves all existing rows up one index (from row 1 to the last, moving the contents of i+1 into i)
  for (int i = 0; i < DISPLAY_MAX_LINES - 1; i++)
  {
    DispParam.tParam.dLines[i] = DispParam.tParam.dLines[i + 1];
  }
  // 2. Insert the new line into the queue line (DISPLAY_MAX_LINES - 1) This overwrites the previously moved content.
  DispParam.tParam.dLines[DISPLAY_MAX_LINES - 1] = text;
  // 3. Re-render the text if auto-scrolling is not active. If it is active, displaytick() will handle the update..
  if (!DispParam.tParam.dAutoScrollOn)
  {
    displayRenderTextLines(DispParam.tParam.dInv, DispParam.tParam.dTrunc);
  }
}

/*-- Scrolling / Pagination --*/

/**
 * @brief Stops the automatic scrolling process.
 */
void displayStopAutoScroll(void) { DispParam.tParam.dAutoScrollOn = false; }

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
void displayLoadAutoScroll(uint8_t mode, const String *arr, size_t n, uint8_t size, bool invert, bool truncate, uint16_t delayMs, bool loop)
{
  DispParam.tmp.loaded = true;
  DispParam.tmp.mode = mode;
  DispParam.tmp.arr = arr;
  DispParam.tmp.n = n;
  DispParam.tmp.size = size;
  DispParam.tmp.invert = invert;
  DispParam.tmp.truncate = truncate;
  DispParam.tmp.delayMs = delayMs;
  DispParam.tmp.loop = loop;
}

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
void displayStartAutoScroll(uint8_t mode, const String *arr, size_t n, uint8_t size, bool invert, bool truncate, uint16_t delayMs, bool loop)
{
  // empty? clear display
  if (n == 0)
  {
    displayClearLines();
    displayClear(true);
    displayStopAutoScroll();
    return;
  }

  displaySetTextSize(size);
  displaySetLines(arr, n);
  DispParam.tParam.dInv = invert;
  DispParam.tParam.dTrunc = truncate;
  DispParam.tParam.dLoop = loop;
  DispParam.tParam.dMode = mode;

  if (DispParam.tParam.dMode == DISPLAY_SCROLL_MODE_NONE) // No scroll
  {
    DEBUG_PRINTLN("DISPLAY_SCROLL_MODE_NONE ");
    displayRenderTextLines(invert, truncate);
    displayStopAutoScroll();
    return;
  }

  const uint8_t vis = displayGetMaxVisibleLines();
  DispParam.tParam.dTotalPages = (DispParam.tParam.dLinesCount == 0) ? 1 : ((DispParam.tParam.dLinesCount + vis - 1) / vis); // ceil

  if (DispParam.tParam.dMode == DISPLAY_SCROLL_MODE_PAGES)
  {
    DispParam.tParam.dDelayMs = delayMs ? delayMs : 1;
    DEBUG_PRINTLN("DISPLAY_SCROLL_MODE_PAGES ");
  }
  else
  { // DISPLAY_SCROLL_MODE_LINES
    DispParam.tParam.dSpeedPx = DISPLAY_SCROLL_PIXEL;
    DispParam.tParam.dPauseMs = DISPLAY_SCROLL_PAUSE;
    DispParam.tParam.dYOffsetPx = 0;
    DispParam.tParam.dTopLineIdx = 0;
    DispParam.tParam.dPendingPause = false;
    DispParam.tParam.dDelayMs = delayMs ? delayMs : 16;
    DEBUG_PRINTLN("DISPLAY_SCROLL_MODE_LINES ");
  }

  DispParam.tParam.dCurrentPage = 0;
  DispParam.tParam.dAutoScrollOn = ((DispParam.tParam.dTotalPages == 1) && (DispParam.tParam.dLoop == false)) ? false : true;
  DispParam.tParam.dLastSwitchMs = millis();
  displayRenderPage(DispParam.tParam.dCurrentPage);
}

/**
 * @brief Renders a specific page of text to the display.
 *
 * @details This function clears the display, sets the text properties (color, size),
 * and prints the lines corresponding to the specified page index.
 *
 * @param pageIndex The 0-indexed page number to render.
 */
void displayRenderPage(uint8_t pageIndex)
{
  disPlay->clearDisplay();
  if (DispParam.tParam.dInv)
  {
    disPlay->fillRect(0, 0, DispParam.dWidth, DispParam.dHeigth, WHITE);
    disPlay->setTextColor(BLACK);
  }
  else
  {
    disPlay->setTextColor(WHITE);
  }
  disPlay->setTextSize(DispParam.tParam.dTextSize);

  const uint8_t vis = displayGetMaxVisibleLines();
  const uint8_t cols = displayGetMaxColsPerLine();
  const int stepY = DispParam.dBaseCharH * DispParam.tParam.dTextSize;
  const uint16_t start = pageIndex * vis;

  for (uint8_t i = 0; i < vis; ++i)
  {
    uint16_t idx = start + i;
    if (idx >= DispParam.tParam.dLinesCount)
      break;
    disPlay->setCursor(0, i * stepY);
    if (DispParam.tParam.dTrunc && DispParam.tParam.dLines[idx].length() > cols)
    {
      String tmp = DispParam.tParam.dLines[idx].substring(0, cols);
      disPlay->print(tmp);
    }
    else
    {
      disPlay->print(DispParam.tParam.dLines[idx]);
    }
  }
  disPlay->display();
}

/**
 * @brief Renders a specific line of text to the display.
 *
 * @details This function clears the display, sets the text properties (color, size),
 * and prints the lines corresponding to the specified pixel index.
 */
void displayRenderScrolled(void)
{
  const uint8_t vis = displayGetMaxVisibleLines();
  const uint8_t cols = displayGetMaxColsPerLine();
  const int lineH = DispParam.dBaseCharH * DispParam.tParam.dTextSize;

  disPlay->clearDisplay();
  if (DispParam.tParam.dInv)
  {
    disPlay->fillRect(0, 0, DispParam.dWidth, DispParam.dHeigth, WHITE);
    disPlay->setTextColor(BLACK);
  }
  else
  {
    disPlay->setTextColor(WHITE);
  }
  disPlay->setTextSize(DispParam.tParam.dTextSize);

  int y = -DispParam.tParam.dYOffsetPx;
  uint8_t idx = DispParam.tParam.dTopLineIdx;

  for (uint8_t i = 0; i < vis + 1; ++i)
  {
    if (idx >= DispParam.tParam.dLinesCount)
    {
      if (!DispParam.tParam.dLoop)
        break; 
      idx = 0; 
    }
    if (y >= DispParam.dHeigth)
      break; 

    if (y + lineH > 0)
    { 
      disPlay->setCursor(0, y);
      const String &s = DispParam.tParam.dLines[idx];
      if (DispParam.tParam.dTrunc && s.length() > cols)
      {
        disPlay->print(s.substring(0, cols));
      }
      else
      {
        disPlay->print(s);
      }
    }

    y += lineH;
    ++idx;
  }

  disPlay->display();
}

/**
 * @brief A non-blocking periodic function to manage automatic scrolling.
 *
 * @details This function should be called repeatedly in the main loop (`loop()`). It checks
 * if auto-scrolling is enabled and if the scroll delay has elapsed. If so, it advances to
 * the next page or loops back to the start.
 */
void displaytick(void)
{

  if (DispParam.tmp.loaded)
  {
    DispParam.tmp.loaded = false;
    displayStartAutoScroll(DispParam.tmp.mode, DispParam.tmp.arr, DispParam.tmp.n, DispParam.tmp.size, DispParam.tmp.invert, DispParam.tmp.truncate, DispParam.tmp.delayMs, DispParam.tmp.loop);
    return;
  }
  // Tempo di partenza della pausa tra righe (solo line-scroll)
  static uint32_t s_pauseStartMs = 0;

  if ((!DispParam.tParam.dAutoScrollOn) || (DispParam.tParam.dMode == DISPLAY_SCROLL_MODE_NONE))
    return;

  const uint32_t now = millis();
  if ((uint32_t)(now - DispParam.tParam.dLastSwitchMs) < DispParam.tParam.dDelayMs)
    return;
  DispParam.tParam.dLastSwitchMs = now;

  /*-- CHANGE PAGE --*/
  if (DispParam.tParam.dMode == DISPLAY_SCROLL_MODE_PAGES)
  {
    if (DispParam.tParam.dCurrentPage + 1 < DispParam.tParam.dTotalPages)
    {
      DispParam.tParam.dCurrentPage++;
      displayRenderPage(DispParam.tParam.dCurrentPage);
    }
    else
    {
      if (DispParam.tParam.dLoop)
      {
        DispParam.tParam.dCurrentPage = 0;
        displayRenderPage(DispParam.tParam.dCurrentPage);
      }
      else
      {
        DispParam.tParam.dAutoScrollOn = false; // finished, stay on the last one
      }
    }
    return;
  }

  /*-- LINE-SCROLL --*/
  const uint8_t lines = DispParam.tParam.dLinesCount;
  const uint8_t vis = displayGetMaxVisibleLines();
  const int lineH = DispParam.dBaseCharH * DispParam.tParam.dTextSize;
  const uint8_t lastTopIdx = (lines > vis) ? (uint8_t)(lines - vis) : 0;

  // Nothing to scroll: ALWAYS stop (even if loop=true)
  if (DispParam.tParam.dLoop && (lines <= vis))
  {
    DispParam.tParam.dYOffsetPx = 0;
    DispParam.tParam.dTopLineIdx = 0;
    DispParam.tParam.dAutoScrollOn = false;
    displayRenderScrolled();
    return;
  }

  // Pause between line breaks (if enabled)
  if (DispParam.tParam.dPendingPause)
  {
    if (DispParam.tParam.dPauseMs == 0 ||
        (uint32_t)(now - s_pauseStartMs) >= DispParam.tParam.dPauseMs)
    {
      DispParam.tParam.dPendingPause = false;
    }
    else
    {
      displayRenderScrolled(); // show static frame during pause
      return;
    }
  }

  // Nothing to scroll: with loop=false I stop immediately
  if (!DispParam.tParam.dLoop && lines <= vis)
  {
    DispParam.tParam.dAutoScrollOn = false;
    displayRenderScrolled();
    return;
  }

  const int step = (DispParam.tParam.dSpeedPx ? DispParam.tParam.dSpeedPx : 1);

  // SNAP & STOP: avoid "empty" frames before the stop on the last page
  if (!DispParam.tParam.dLoop &&
      DispParam.tParam.dTopLineIdx == lastTopIdx &&
      (DispParam.tParam.dYOffsetPx + step) >= lineH)
  {
    DispParam.tParam.dYOffsetPx = 0;        
    DispParam.tParam.dAutoScrollOn = false; 
    displayRenderScrolled();                
    return;
  }

  // Advance vertical offset in pixels
  DispParam.tParam.dYOffsetPx += step;

  // Line snap when you exceed the line height
  if (DispParam.tParam.dYOffsetPx >= lineH)
  {
    // preserves any residue (supports speedPx > lineH)
    DispParam.tParam.dYOffsetPx %= lineH;

    if (DispParam.tParam.dLoop)
    {
      // CONTINUOUS LOOP: top index scrolls 0..lines-1 with wrap
      if (lines)
      {
        DispParam.tParam.dTopLineIdx = (DispParam.tParam.dTopLineIdx + 1) % lines;
      }
    }
    else
    {
      if (DispParam.tParam.dTopLineIdx < lastTopIdx)
      {
        DispParam.tParam.dTopLineIdx++;
      }
      else
      {
        DispParam.tParam.dYOffsetPx = 0;
        DispParam.tParam.dAutoScrollOn = false;
        displayRenderScrolled();
        return;
      }
    }

    // Pausa opzionale a ogni aggancio riga
    if (DispParam.tParam.dPauseMs > 0)
    {
      DispParam.tParam.dPendingPause = true;
      s_pauseStartMs = now;
    }
  }

  // Draw the current frame (line-scroll)
  displayRenderScrolled();
}