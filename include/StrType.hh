/**
 * @file StrType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct for representing python strings
 * @version 0.1
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_StrType_
#define PythonMonkey_StrType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents the 'string' type in Python, which is represented as a 'char*' in C++. It inherits from the PyType struct
 */
struct StrType : public PyType {
public:
  StrType(PyObject *object);
  StrType(char *string);

  /**
   * @brief Construct a new StrType object from a JSString. Automatically handles encoding conversion for latin1 & UCS2:
   * codepoint     | Python          | Spidermonkey     | identical representation?
   * 000000-0000FF | latin1          | latin1           | Yes
   * 000100-00D7FF | UCS2            | UTF16            | Yes
   * 00D800-00DFFF | UCS2 (unpaired) | UTF16 (unpaired) | Yes
   * 00E000-00FFFF | UCS2            | UTF16            | Yes
   * 010000-10FFFF | UCS4            | UTF16            | No, conversion and new backing store required, user must explicitly call asUCS4()
   *
   * @param cx - javascript context pointer
   * @param str - JSString pointer
   */
  StrType(JSContext *cx, JSString *str);

  const TYPE returnType = TYPE::STRING;
  const char *getValue() const;

  /**
   * @brief creates new UCS4-encoded pyObject string. This must be called by the user if the original JSString contains any surrogate pairs
   *
   * @return PyObject* - the UCS4-encoding of the pyObject string
   *
   */
  static PyObject *asUCS4(PyObject *pyObject);
};

#endif