/**
 * @file PyBytesProxyHandler.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS Uint8Array-like proxy objects for immutable bytes objects
 * @date 2024-07-23
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyBytesProxy_
#define PythonMonkey_PyBytesProxy_


#include "include/PyObjectProxyHandler.hh"


/**
 * @brief This struct is the ProxyHandler for JS Proxy Iterable pythonmonkey creates to handle coercion from python iterables to JS Objects
 *
 */
struct PyBytesProxyHandler : public PyObjectProxyHandler {
public:
  PyBytesProxyHandler() : PyObjectProxyHandler(&family) {};
  static const char family;

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

  bool getOwnPropertyDescriptor(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
  ) const override;  

  /**
   * @brief Handles python object reference count when JS Proxy object is finalized
   *
   * @param gcx pointer to JS::GCContext
   * @param proxy the proxy object being finalized
   */
  void finalize(JS::GCContext *gcx, JSObject *proxy) const override;
};

#endif