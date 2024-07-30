/**
 * @file PyIterableProxyHandler.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects for iterables
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyIterableProxy_
#define PythonMonkey_PyIterableProxy_


#include "include/PyObjectProxyHandler.hh"


/**
 * @brief This struct is the ProxyHandler for JS Proxy Iterable pythonmonkey creates to handle coercion from python iterables to JS Objects
 *
 */
struct PyIterableProxyHandler : public PyObjectProxyHandler {
public:
  PyIterableProxyHandler() : PyObjectProxyHandler(&family) {};
  static const char family;

  bool getOwnPropertyDescriptor(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
  ) const override;
};

#endif