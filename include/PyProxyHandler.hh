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
 * @brief This struct is the ProxyHandler for JS Proxy Objects pythonmonkey creates to handle coercion from python dicts to JS Objects
 *
 */
struct PyProxyHandler : public js::BaseProxyHandler {
public:
  PyProxyHandler(PyObject *pyObject);
  PyObject *pyObject; // @TODO (Caleb Aikens) Consider putting this in a private slot

  /**
   * @brief [[OwnPropertyKeys]]
   *
   * @param cx
   * @param proxy
   * @param props
   * @return true
   * @return false
   */
  bool ownPropertyKeys(JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;
  /**
   * @brief [[Delete]]
   *
   * @param cx
   * @param proxy
   * @param id
   * @param result
   * @return true
   * @return false
   */
  bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::ObjectOpResult &result) const override;
  /**
   * @brief [[HasProperty]]
   *
   * @param cx
   * @param proxy
   * @param id
   * @param bp
   * @return true
   * @return false
   */
  bool has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    bool *bp) const override;
  /**
   * @brief [[Get]]
   *
   * @param cx
   * @param proxy
   * @param receiver
   * @param id
   * @param vp
   * @return true
   * @return false
   */
  bool get(JSContext *cx, JS::HandleObject proxy,
    JS::HandleValue receiver, JS::HandleId id,
    JS::MutableHandleValue vp) const override;
  /**
   * @brief [[Set]]
   *
   * @param cx
   * @param proxy
   * @param id
   * @param v
   * @param receiver
   * @param result
   * @return true
   * @return false
   */
  bool set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    JS::HandleValue v, JS::HandleValue receiver,
    JS::ObjectOpResult &result) const override;
  /**
   * @brief [[Enumerate]]
   *
   * @param cx
   * @param proxy
   * @param props
   * @return true
   * @return false
   */
  bool enumerate(JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;

  // @TODO (Caleb Aikens) The following are Spidermonkey-unique extensions, need to read into them more
  bool hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
    bool *bp) const override;
  bool getOwnEnumerablePropertyKeys(
    JSContext *cx, JS::HandleObject proxy,
    JS::MutableHandleIdVector props) const override;
  void trace(JSTracer *trc, JSObject *proxy) const override;
  void finalize(JS::GCContext *gcx, JSObject *proxy) const override;
};

#endif
