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
 * @file functionkeys.cpp
 * @brief Implementation for managing virtual function keys.
 *
 * This file contains the implementation of the functions declared in `functionkeys.h`.
 * It manages the association and execution of functions (callbacks) via a
 * virtual "function key" mechanism.
 */

#include "functionkeys.h"

/**
 * @var static fn_ptr_t fnExecutables[FN_MAX]
 * @brief Array of function pointers associated with each function key.
 *
 * `fnExecutables[i]` contains the pointer to the function that should be
 * executed when function key `i` is active. Initialized to `NULL`.
 */
static fn_ptr_t fnExecutables[FN_MAX] = {NULL};

/**
 * @brief Array to store the state (ON/OFF) of each function key.
 *
 * `fnState[i]` is `true` if function key `i` is active, `false` otherwise.
 */
static bool fnState[FN_MAX] = {false};

/**
 * @brief Initializes the FN state.
 *
 * Resets the state of all function keys, setting them to `false`.
 */
void fnInit()
{
  for (size_t i = 0; i < FN_MAX; i++)
    fnState[i] = false;
}

/**
 * @brief Registration function to associate a function pointer with an index.
 *
 * Associates the pointer `func` with the index `idx` of the `fnExecutables` array.
 * It does not perform checks on the index except to prevent overflow.
 * @param idx The index of the function key to associate the function with.
 * @param func The function pointer to register.
 */
void fnRegister_fix(size_t idx, fn_ptr_t func)
{
  if (idx < FN_MAX)
    fnExecutables[idx] = func;
}

/**
 * @brief Automatic registration function for a function in the first available slot.
 *
 * Searches for the first free slot (where the pointer is `NULL`) and registers the function.
 * @param func The function pointer to register.
 * @return The index of the slot where the function was registered, or -1 if there are no available slots.
 */
int fnRegister(fn_ptr_t func)
{
  for (size_t i = 0; i < FN_MAX; ++i)
  {
    if (fnExecutables[i] == NULL)
    {
      fnExecutables[i] = func;
      return i;
    }
  }

  return -1;
}

/**
 * @brief Sets the ON/OFF state of the function key.
 *
 * Sets the state of the function key `idx` to `on`. If the index is out of range,
 * the function does nothing.
 * @param idx The index of the function key to modify.
 * @param on The new state (`true` for active, `false` for inactive).
 */
void fnSet(size_t idx, bool on)
{
  if (idx >= FN_MAX)
    return;
#ifdef FN_EXCLUSIVE
  for (size_t i = 0; i < FN_MAX; i++)
  {
    fnState[i] = false;
  }
#endif
  fnState[idx] = on;
}

/**
 * @brief Periodically executes the actions associated with active FNs.
 *
 * Iterates through the `fnState` array. If a function key is active (`fnState[i]` is `true`)
 * and has a registered function (`fnExecutables[i]` is not `NULL`), it executes the function.
 */
void fnExecuteTick()
{
  for (size_t i = 0; i < FN_MAX; i++)
  {
    if (fnState[i] && fnExecutables[i] != NULL)
      fnExecutables[i](); // Esegui la funzione puntata dall'array
  }
}
