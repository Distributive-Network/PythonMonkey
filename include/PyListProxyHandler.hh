/**
 * @file PyListProxyHandler.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Structs for creating JS proxy objects. Used by ListType for List coercion
 * @version 0.1
 * @date 2023-12-01
 *
 * Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyListProxy_
#define PythonMonkey_PyListProxy_

#include "PyBaseProxyHandler.hh"


/**
 * @brief This struct is the ProxyHandler for JS Proxy Objects pythonmonkey creates
 *    to handle coercion from python lists to JS Array objects
 */
struct PyListProxyHandler : public PyBaseProxyHandler {
public:
  PyListProxyHandler(PyObject *pyObj) : PyBaseProxyHandler(pyObj, &family) {};
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
