/**
 * @file JSStringProxy.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSStringProxy is a custom C-implemented python type that derives from str. It acts as a proxy for JSStrings from Spidermonkey, and behaves like a str would.
 * @date 2024-01-03
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSStringProxy_
#define PythonMonkey_JSStringProxy_

#include <jsapi.h>

#include <Python.h>

/**
 * @brief The typedef for the backing store that will be used by JSStringProxy objects. All it contains is a pointer to the JSString
 *
 */
typedef struct {
  PyUnicodeObject str;
  JS::PersistentRootedValue jsString;
} JSStringProxy;

/**
 * @brief Struct for the JSStringProxyType, used by all JSStringProxy objects
 */
extern PyTypeObject JSStringProxyType;

#endif