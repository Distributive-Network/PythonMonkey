/**
 * @file PyObjectProxyHandler.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Structs for creating JS proxy objects. Used for default object coercion
 * @date 2024-01-25
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyObjectProxy_
#define PythonMonkey_PyObjectProxy_

#include "include/PyBaseProxyHandler.hh"
#include <jsapi.h>
#include <js/Proxy.h>

#include <Python.h>

/**
 * @brief This struct is the ProxyHandler for JS Proxy Objects pythonmonkey creates to handle coercion from python objects to JS Objects
 *
 */
struct PyObjectProxyHandler : public PyBaseProxyHandler {
public:
  PyObjectProxyHandler(PyObject *pyObj) : PyBaseProxyHandler(pyObj, &family) {};
  static const char family;

  /**
   * @brief [[OwnPropertyKeys]]
   *
   * @param cx - pointer to JSContext
   * @param proxy - The proxy object who's keys we output
   * @param props - out-parameter of object IDs
   * @return true - call succeeded
   * @return false - call failed and an exception has been raised
   */
  bool ownPropertyKeys(JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;
  /**
   * @brief [[Delete]]
   *
   * @param cx - pointer to JSContext
   * @param proxy - The proxy object who's property we wish to delete
   * @param id - The key we wish to delete
   * @param result - operation result
   * @return true - call succeeded
   * @return false - call failed and an exception has been raised
   */
  bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::ObjectOpResult &result) const override;
  /**
   * @brief [[HasProperty]]
   *
   * @param cx - pointer to JSContext
   * @param proxy - The proxy object who's propery we wish to check
   * @param id - key value of the property to check
   * @param bp - out-paramter: true if object has property, false if not
   * @return true - call succeeded
   * @return false - call failed and an exception has been raised
   */
  bool has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    bool *bp) const override;
  /**
   * @brief [[Set]]
   *
   * @param cx pointer to JSContext
   * @param proxy The proxy object who's property we wish to set
   * @param id Key of the property we wish to set
   * @param v Value that we wish to set the property to
   * @param result operation result
   * @return true call succeed
   * @return false call failed and an exception has been raised
   */
  bool set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::HandleValue v, JS::HandleValue receiver,
    JS::ObjectOpResult &result) const override;
  /**
   * @brief [[Enumerate]]
   *
   * @param cx - pointer to JSContext
   * @param proxy - The proxy object who's keys we output
   * @param props - out-parameter of object IDs
   * @return true - call succeeded
   * @return false - call failed and an exception has been raised
   */
  bool enumerate(JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;
  /**
   * @brief @TODO (Caleb Aikens) read up on what this trap does exactly
   *
   * @param cx pointer to JSContext
   * @param proxy The proxy object who's property we wish to check
   * @param id  Key of the property we wish to check
   * @param bp out-paramter: true if object has property, false if not
   * @return true call succeeded
   * @return false call failed and an exception has been raised
   */
  bool hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    bool *bp) const override;
  /**
   *
   * @param cx - pointer to JSContext
   * @param proxy - The proxy object who's keys we output
   * @param props - out-parameter of object IDs
   * @return true - call succeeded
   * @return false - call failed and an exception has been raised
   */
  bool getOwnEnumerablePropertyKeys(
    JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;
  /**
   * @brief Handles python object reference count when JS Proxy object is finalized
   *
   * @param gcx pointer to JS::GCContext
   * @param proxy the proxy object being finalized
   */
  void finalize(JS::GCContext *gcx, JSObject *proxy) const override;

  bool getOwnPropertyDescriptor(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
  ) const override;

  bool defineProperty(JSContext *cx, JS::HandleObject proxy,
    JS::HandleId id,
    JS::Handle<JS::PropertyDescriptor> desc,
    JS::ObjectOpResult &result) const override;
};

#endif