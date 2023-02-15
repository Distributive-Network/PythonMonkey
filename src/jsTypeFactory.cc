/**
 * @file jsTypeFactory.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
 * @version 0.1
 * @date 2023-02-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/jsTypeFactory.hh"

#include "include/BoolType.hh"
#include "include/FloatType.hh"
#include "include/IntType.hh"
#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <jsapi.h>

JS::Value jsTypeFactory(PyType *pyType) {
  JS::Value returnType;
  switch (pyType->returnType)
  {
  case TYPE::BOOL:
    returnType.setBoolean(((BoolType *)pyType)->getValue());
    break;
  case TYPE::INT:
    returnType.setNumber(((IntType *)pyType)->getValue());
  case TYPE::FLOAT:
    returnType.setNumber(((FloatType *)pyType)->getValue());
  default:
    if (pyType->getPyObject() == Py_None) {
      returnType.setUndefined();
    }
    else {
      PyErr_SetString(PyExc_TypeError, "");
    }
  }
  return returnType;

}