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
 * @file functionkeys.h
 * @brief Declarations for managing virtual function keys.
 *
 * This file defines the interface for a module that allows associating and
 * managing functions (callbacks) with virtual "function keys," numbered from 0 to 7.
 * It's useful for periodically executing specific actions when a given
 * function key is "active."
 */
 
#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @def FN_MAX
 * @brief The maximum number of function keys managed.
 *
 * Defines the maximum size of the internal arrays used to
 * store the state and function pointers.
 */
#define FN_MAX 8

/**
 * @def FN_EXCLUSIVE
 * @brief If defined, the functions are mutually exclusive: activating one disables all the others.
 *
 */
#define FN_EXCLUSIVE    

/**
 * @typedef fn_ptr_t
 * @brief Type for a function pointer with no arguments and no return value.
 *
 * This data type is used to define the functions that can be
 * associated with the function keys. The functions must have the signature `void func()`.
 */
typedef void (*fn_ptr_t)(void);

/**
 * @brief Initializes the function key module.
 *
 * Resets the state of all function keys to "off" (false).
 * This function must be called before using any other
 * function in the module.
 */
void fnInit();

/**
 * @brief Registers a function at a fixed index.
 *
 * Associates a function pointer (`func`) with the specified index (`idx`).
 * If the index is outside the valid range, the function does nothing.
 *
 * @param idx The index of the function key (from 0 to `FN_MAX` - 1).
 * @param func The function pointer to associate.
 */
void fnRegister_fix(size_t idx, fn_ptr_t func);

/**
 * @brief Automatically registers a function in the first available slot.
 *
 * Searches for the first free slot (where the function pointer is `NULL`) and
 * associates the provided function with it.
 *
 * @param func The function pointer to register.
 * @return The index of the slot where the function was registered,
 * or -1 if no slots are available.
 */
int fnRegister(fn_ptr_t func);

/**
 * @brief Sets the ON/OFF state of a function key.
 *
 * Activates or deactivates the function key specified by the index. If the
 * key is "on" (`true`), its associated function will be executed
 * by `fnExecuteTick()`.
 *
 * @param idx The index of the function key (from 0 to `FN_MAX` - 1).
 * @param on The state to set: `true` for active, `false` for inactive.
 */
void fnSet(size_t idx, bool on);

/**
 * @brief Periodically executes actions associated with active function keys.
 *
 * This function iterates through all function keys. If a key is active
 * (its state is `true`) and has a registered function, it executes that function.
 * It is intended to be called within the main program loop.
 */
void fnExecuteTick();
