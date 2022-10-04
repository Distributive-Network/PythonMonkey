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

#include <Python.h>

#include <iostream>

/**
 * @brief This struct represents a dictionary in python. It derives from the PyType struct
 *
 * @author Giovanni
 */
struct DictType : public PyType {
public:
  DictType(PyObject *object);
  const TYPE returnType = TYPE::DICT;
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

  void print_helper(std::ostream &os, int depth = 0) const;

protected:
  virtual void print(std::ostream &os) const override;
};

#endif