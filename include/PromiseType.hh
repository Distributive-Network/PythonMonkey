/**
 * @file PromiseType.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief Struct for representing Promises
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PythonMonkey_PromiseType_
#define PythonMonkey_PromiseType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>
#include <js/Promise.h>

#include <Python.h>

/**
 * @brief This struct represents the JS Promise type in Python using our custom pythonmonkey.promise type. It inherits from the PyType struct
 */
struct PromiseType : public PyType {
public:
  PromiseType(PyObject *object);

  /**
   * @brief Construct a new PromiseType object from a JS::PromiseObject.
   *
   * @param cx - javascript context pointer
   * @param promise - JS::PromiseObject to be coerced
   */
  PromiseType(JSContext *cx, JS::HandleObject promise);

  const TYPE returnType = TYPE::PYTHONMONKEY_PROMISE;
protected:
  virtual void print(std::ostream &os) const override;
};

#endif