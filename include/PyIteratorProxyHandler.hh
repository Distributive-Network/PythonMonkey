/**
 * @file PyObjectProxyHandler.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy iterators.
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyIteratorProxy_
#define PythonMonkey_PyIteratorProxy_


#include "include/PyObjectProxyHandler.hh"


/**
 * @brief This struct is the ProxyHandler for JS Proxy Iterator pythonmonkey creates to handle coercion from python iterators to JS Objects
 *
 */
struct PyIteratorProxyHandler : public PyObjectProxyHandler {
public:
  PyIteratorProxyHandler() : PyObjectProxyHandler(&family) {};
  static const char family;

  /**
   * @brief Helper function to return next item in iteration
   *
   * @param cx - pointer to the JSContext
   * @param argc - unused
   * @param vp - unused
   * @return true - this function returns true for success and false for failure
   */
  static bool iterator_next(JSContext *cx, unsigned argc, JS::Value *vp);

  bool getOwnPropertyDescriptor(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
  ) const override;

  /**
   * @brief An array of method definitions for Iterator prototype methods
   *
   */
  static JSMethodDef iterator_methods[];
};

#endif