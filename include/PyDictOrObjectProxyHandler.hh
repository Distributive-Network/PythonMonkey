/**
 * @file PyDictOrObjectProxyHandler.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Abstract class inherited by PyDictProxyHandler and PyObjectProxyHandler since they share a lot of logic
 * @date 2024-02-13
 *
 * Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyDictOrObjectProxyHandler_
#define PythonMonkey_PyDictOrObjectProxyHandler_

#include "include/PyBaseProxyHandler.hh"

#include <jsapi.h>

#include <Python.h>

struct PyDictOrObjectProxyHandler : public PyBaseProxyHandler {
  PyDictOrObjectProxyHandler(const void *family) : PyBaseProxyHandler(family) {};

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
   * @brief An array of method definitions for Object prototype methods
   *
   */
  static JSMethodDef object_methods[];
};

#endif