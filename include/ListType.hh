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

#include <Python.h>

/**
 * @brief This struct represents a list in python. It derives from the PyType struct
 *
 * @author Giovanni
 */
struct ListType : public PyType {
public:
  ListType();
  ListType(PyObject *object);
  const TYPE returnType = TYPE::LIST;
/**
 * @brief
 *
 *
 * @param index The index of the list item
 * @param value The value of the list item
 */
  void set(int index, PyType *value);

/**
 * @brief Gets the list item at the given index
 *
 * @param index The index of the item in question
 * @return PyType* Returns a pointer to the appropriate PyType object
 */
  PyType *get(int index) const;

/**
 * @brief Appends the given value to the list
 *
 * @param value The item to be appended
 */
  void append(PyType *value);

/**
 * @brief
 *
 *
 *
 * @returns int length of the list
 */
  int len() const;

  void sort();
};
#endif