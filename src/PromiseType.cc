/**
 * @file PromiseType.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Struct for representing Promises
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/PromiseType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <jsapi.h>
#include <js/Promise.h>

#include <Python.h>

PromiseType::PromiseType(PyObject *object) : PyType(object) {}

PromiseType::PromiseType(JSContext *cx, JS::Handle<JSObject *> *promise) {}

