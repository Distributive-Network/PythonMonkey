/**
 * @file ListType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct for representing python lists
 * @version 0.1
 * @date 2022-08-18
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_ListType_
#define PythonMonkey_ListType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>


/**
 * @brief This struct represents a list in python. It derives from the PyType struct
 *
 * @author Giovanni
 */
struct ListType : public PyType {
public:
  ListType();
  ListType(PyObject *object);
  ListType(JSContext *cx, JS::HandleObject arrayObj);
  const TYPE returnType = TYPE::LIST;
};
#endif