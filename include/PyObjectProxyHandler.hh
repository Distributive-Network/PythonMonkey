/**
 * @file PyObjectProxyHandler.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for creating JS proxy objects for all objects
 * @date 2024-01-25
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
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
  PyObjectProxyHandler() : PyBaseProxyHandler(&family) {};
  PyObjectProxyHandler(const void *childFamily) : PyBaseProxyHandler(childFamily) {};
  static const char family;

  /**
   * @brief Helper function used by dicts and objects for ownPropertyKeys
   *
   * @param cx - pointer to the JSContext
   * @param keys - PyListObject containing the keys of the proxy'd dict/object
   * @param length - the length of keys param
   * @param props - out-param, will be a JS vector of the keys converted to JS Ids
   * @return true - the function succeeded
   * @return false - the function failed (an Exception should be raised)
   */
  static bool handleOwnPropertyKeys(JSContext *cx, PyObject *keys, size_t length, JS::MutableHandleIdVector props);

  /**
   * @brief Helper function used by dicts and objects for get OwnPropertyDescriptor
   *
   * @param cx - pointer to the JSContext
   * @param id - id of the prop to get
   * @param desc - out-param, the property descriptor
   * @param item - the python object to be converted to a JS prop
   * @return true - the function succeeded
   * @return false - the function has failed and an exception has been raised
   */
  static bool handleGetOwnPropertyDescriptor(JSContext *cx, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc, PyObject *item);

  /**
   * @brief Handles python object reference count when JS Proxy object is finalized
   *
   * @param gcx pointer to JS::GCContext
   * @param proxy the proxy object being finalized
   */
  void finalize(JS::GCContext *gcx, JSObject *proxy) const override;

  /**
   * @brief Helper function used by dicts and objects to convert dict/object to String
   *
   * @param cx - pointer to the JSContext
   * @param argc - unused
   * @param vp - unused
   * @return true - this function always returns true
   */
  static bool object_toString(JSContext *cx, unsigned argc, JS::Value *vp);

  /**
   * @brief Helper function used by dicts and objects to convert dict/object to LocaleString
   *
   * @param cx - pointer to the JSContext
   * @param argc - unused
   * @param vp - unused
   * @return true - this function always returns true
   */
  static bool object_toLocaleString(JSContext *cx, unsigned argc, JS::Value *vp);

  /**
   * @brief Helper function used by dicts and objects to get valueOf, just returns a new reference to `self`
   *
   * @param cx - pointer to the JSContext
   * @param argc - unused
   * @param vp - unused
   * @return true - the function succeeded
   * @return false - the function failed and an exception has been raised
   */
  static bool object_valueOf(JSContext *cx, unsigned argc, JS::Value *vp);

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
   * @param result - whether the call succeeded or not
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
   * @param receiver The `this` value to use when executing any code
   * @param result whether or not the call succeeded
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
   * @brief  Returns true if `id` is in `proxy`, false otherwise
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
   * @brief Returns vector of proxy's own keys
   *
   * @param cx - Pointer to the JSContext
   * @param proxy - the proxy object
   * @param props - out parameter, the vector of proxy's own keys
   * @return true - the call succeeded
   * @return false - the call failed and an exception has been raised
   */
  bool getOwnEnumerablePropertyKeys(
    JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;

  bool getOwnPropertyDescriptor(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
  ) const override;

  bool defineProperty(JSContext *cx, JS::HandleObject proxy,
    JS::HandleId id,
    JS::Handle<JS::PropertyDescriptor> desc,
    JS::ObjectOpResult &result) const override;

  bool getBuiltinClass(JSContext *cx, JS::HandleObject proxy, js::ESClass *cls) const override;
};

#endif