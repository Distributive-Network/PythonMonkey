/**
 * @file DateType.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing python dates
 * @version 0.1
 * @date 2022-12-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_DateType_
#define PythonMonkey_DateType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>
#include <js/Date.h>

#include <Python.h>

#include <iostream>

/**
 * @brief This struct represents the 'datetime' type in Python from the datetime module, which is represented as a 'Date' object in JS. It inherits from the PyType struct
 */
struct DateType : public PyType {
public:
  DateType(PyObject *object);
  DateType(JSContext *cx, JS::Handle<JSObject *> dateObj);
  TYPE getReturnType() override;
};

#endif