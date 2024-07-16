/**
 * @file StrType.hh
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python strings
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_StrType_
#define PythonMonkey_StrType_

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents the 'string' type in Python, which is represented as a 'char*' in C++
 */
struct StrType {
public:
  /**
   * @brief Construct a new unicode PyObject from a JSString. Automatically handles encoding conversion for latin1 & UCS2:
   * codepoint     | Python          | Spidermonkey     | identical representation?
   * 000000-0000FF | latin1          | latin1           | Yes
   * 000100-00D7FF | UCS2            | UTF16            | Yes
   * 00D800-00DFFF | UCS2 (unpaired) | UTF16 (unpaired) | Yes
   * 00E000-00FFFF | UCS2            | UTF16            | Yes
   * 010000-10FFFF | UCS4            | UTF16            | No, conversion and new backing store required, user must explicitly call asUCS4() -> static in code
   *
   * @param cx - javascript context pointer
   * @param str - JSString pointer
   *
   * @returns PyObject* pointer to the resulting PyObject
   */
  static PyObject *getPyObject(JSContext *cx, JS::HandleValue str);

  static const char *getValue(JSContext *cx, JS::HandleValue str);

  static PyObject *proxifyString(JSContext *cx, JS::HandleValue str);
};

#endif