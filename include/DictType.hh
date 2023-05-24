/**
 * @file DictType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct representing python dictionaries
 * @version 0.1
 * @date 2022-08-10
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_DictType_
#define PythonMonkey_DictType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>

#include <Python.h>

#include <iostream>

/**
 * @brief This struct represents a dictionary in python. It derives from the PyType struct
 *
 * @author Giovanni
 */
struct DictType : public PyType {
public:
  DictType();
  DictType(PyObject *object);

  /**
   * @brief Construct a new DictType object from a JSObject.
   *
   * @param cx - pointer to the JSContext
   * @param global - pointer to the global JSObject
   * @param jsObject - pointer to the JSObject to be coerced
   */
  DictType(JSContext *cx, JS::Handle<JSObject *> global, JS::Handle<JS::Value> jsObject);

  /**
   * @brief Construct a new DictType object from a JSObject, providing a map of JSObjects that have already been coerced to python dicts.
   *
   * @param cx - pointer to the JSContext
   * @param global - pointer to the global JSObject
   * @param jsObject - pointer to the JSObject to be coerced
   * @param subObjectsMap - map of JSObjects that have been coerced to PyObjects
   */
  DictType(JSContext *cx, JS::Handle<JSObject *> global, JS::Handle<JS::Value> jsObject, std::unordered_map<const JS::Value *, PyObject *> &subObjectsMap);

  TYPE getReturnType() override;
/**
 * @brief The 'set' method for a python dictionary. Sets the approprite 'key' in the dictionary with the appropriate 'value'
 *
 * @param key The key of the dictionary item
 * @param value The value of the dictionary item
 */
  void set(PyType *key, PyType *value);

/**
 * @brief Gets the dictionary item at the given 'key'
 *
 * @param key The key of the item in question
 * @return PyType* Returns a pointer to the appropriate PyType object
 */
  PyType *get(PyType *key) const;

  /**
   * @brief
   *
   * @param cx
   * @param jsObject
   * @return PyObject*
   */
  static PyObject *getJSObjectPyProxy(JSContext *cx, JS::HandleObject jsObject);

private:
  /**
   * @brief Helper function for DictType constructor that keeps track of reference cycles
   *
   * @param cx - pointer to the JSContext
   * @param global - pointer to the global JSObject
   * @param jsObject - pointer to the JSObject to be coerced
   * @param subObjectsMap - map of JSObjects that have been coerced to PyObjects
   */
  void init(JSContext *cx, JS::Handle<JSObject *> global, JS::Handle<JS::Value> jsObject, std::unordered_map<const JS::Value *, PyObject *> &subObject);
};

#endif