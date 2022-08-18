#ifndef Bifrost_ListType_
#define Bifrost_ListType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief This class represents a list in python. It derives from the PyType class
 *
 * @author Giovanni
 */
class ListType : public PyType {
public:
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

protected:
virtual void print(std::ostream &os) const override;
};
#endif