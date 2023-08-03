/**
 * @file PyProxy.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for creating JS proxy objects. Used by DictType for object coercion
 * @version 0.1
 * @date 2023-04-20
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyProxy_
#define PythonMonkey_PyProxy_

#include <jsapi.h>
#include <js/Proxy.h>

#include <Python.h>

/**
 * @brief base class for PyProxyHandler and PyListProxyHandler
 */
struct PyBaseProxyHandler : public js::BaseProxyHandler {
public:
  PyBaseProxyHandler(PyObject *pyObj, const void *family) : js::BaseProxyHandler(family), pyObject(pyObj) {};
  PyObject *pyObject; // @TODO (Caleb Aikens) Consider putting this in a private slot

  bool getPrototypeIfOrdinary(JSContext *cx, JS::HandleObject proxy, bool *isOrdinary, JS::MutableHandleObject protop) const override final;
  bool preventExtensions(JSContext *cx, JS::HandleObject proxy, JS::ObjectOpResult &result) const override final;
  bool isExtensible(JSContext *cx, JS::HandleObject proxy, bool *extensible) const override final;
};

/**
 * @brief This struct is the ProxyHandler for JS Proxy Objects pythonmonkey creates to handle coercion from python dicts to JS Objects
 *
 */
struct PyProxyHandler : public PyBaseProxyHandler {
public:
  PyProxyHandler(PyObject *pyObj) : PyBaseProxyHandler(pyObj, &family) {};
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
   * @param result - @TODO (Caleb Aikens) read up on JS::ObjectOpResult
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
   * @brief [[Get]]
   *
   * @param cx pointer to JSContext
   * @param proxy - The proxy object who's property we wish to check
   * @param receiver @TODO (Caleb Aikens) read ECMAScript docs about this
   * @param id - Key of the property we wish to get
   * @param vp - out-paramter for the gotten property
   * @return true - call succeeded
   * @return false - call failed and an exception has been raised
   */
  bool get(JSContext *cx, JS::HandleObject proxy,
    JS::HandleValue receiver, JS::HandleId id,
    JS::MutableHandleValue vp) const override;
  /**
   * @brief [[Set]]
   *
   * @param cx pointer to JSContext
   * @param proxy The proxy object who's property we wish to set
   * @param id Key of the property we wish to set
   * @param v Value that we wish to set the property to
   * @param receiver @TODO (Caleb Aikens) read ECMAScript docs about this
   * @param result @TODO (Caleb Aikens) read ECMAScript docs about this
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

  // @TODO (Caleb Aikens) The following are Spidermonkey-unique extensions, need to read into them more
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
   * @brief @TODO (Caleb Aikens) read up on what this trap does exactly
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

/**
 * @brief This struct is the ProxyHandler for JS Proxy Objects pythonmonkey creates
 *    to handle coercion from python lists to JS Array-like objects
 */
struct PyListProxyHandler : public PyBaseProxyHandler {
public:
  PyListProxyHandler(PyObject *pyObj) : PyBaseProxyHandler(pyObj, &family) {};
  static const char family;

  bool getOwnPropertyDescriptor(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
  ) const override;

  bool defineProperty(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult &result
  ) const override;

  bool ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const override;
  bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult &result) const override;
};

/**
 * @brief Convert jsid to a PyObject to be used as dict keys
 */
PyObject *idToKey(JSContext *cx, JS::HandleId id);

/**
 * @brief Convert Python dict key to jsid
 */
bool keyToId(PyObject *key, JS::MutableHandleId idp);

#endif
