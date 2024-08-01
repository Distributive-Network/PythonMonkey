/**
 * @file PyBaseProxyHandler.hh
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Structs for creating JS proxy objects.
 * @date 2023-04-20
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyBaseProxy_
#define PythonMonkey_PyBaseProxy_

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>

#include <Python.h>

/**
 * @brief base class for PyDictProxyHandler and PyListProxyHandler
 */
struct PyBaseProxyHandler : public js::BaseProxyHandler {
public:
  PyBaseProxyHandler(const void *family) : js::BaseProxyHandler(family) {};

  bool getPrototypeIfOrdinary(JSContext *cx, JS::HandleObject proxy, bool *isOrdinary, JS::MutableHandleObject protop) const override final;
  bool preventExtensions(JSContext *cx, JS::HandleObject proxy, JS::ObjectOpResult &result) const override final;
  bool isExtensible(JSContext *cx, JS::HandleObject proxy, bool *extensible) const override final;
};

enum ProxySlots {PyObjectSlot, OtherSlot};

typedef struct {
  const char *name;      /* The name of the method */
  JSNative call;         /* The C function that implements it */
  uint16_t nargs;        /* The argument count for the method */
} JSMethodDef;

/**
 * @brief Convert jsid to a PyObject to be used as dict keys
 */
PyObject *idToKey(JSContext *cx, JS::HandleId id);

/**
 * @brief Convert Python dict key to jsid
 */
bool keyToId(PyObject *key, JS::MutableHandleId idp);

bool idToIndex(JSContext *cx, JS::HandleId id, Py_ssize_t *index);

#endif