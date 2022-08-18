#ifndef Bifrost_DictType_
#define Bifrost_DictType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief This class represents a dictionary in python. It derives from the PyType class
 *
 * @author Giovanni
 */
class DictType : public PyType {
public:
DictType(PyObject *object);
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
const TYPE returnType = TYPE::DICT;
virtual void print(std::ostream &os) const override;
};

#endif