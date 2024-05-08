/**
 * @file PyListProxyHandler.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects for Lists
 * @date 2023-12-01
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyListProxy_
#define PythonMonkey_PyListProxy_

#include "include/PyBaseProxyHandler.hh"


/**
 * @brief This struct is the ProxyHandler for JS Proxy Objects pythonmonkey creates
 *    to handle coercion from python lists to JS Array objects
 */
struct PyListProxyHandler : public PyBaseProxyHandler {
public:
  PyListProxyHandler() : PyBaseProxyHandler(&family) {};
  static const char family;

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

  bool defineProperty(
    JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult &result
  ) const override;

  bool ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const override;
  bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult &result) const override;
  bool isArray(JSContext *cx, JS::HandleObject proxy, JS::IsArrayAnswer *answer) const override;
  bool getBuiltinClass(JSContext *cx, JS::HandleObject proxy, js::ESClass *cls) const override;
};

#endif
