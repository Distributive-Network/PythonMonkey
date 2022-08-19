/**
 * @file TupleType.hh
 * @author Giovanni Tedesco
 * @brief Class and function prototypes for a class that represents the python tuple type in C++
 * @version 0.1
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef Bifrost_TupleType_
#define Bifrost_TupleType_

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief A class to represent the tuple type in python
 *
 */
class TupleType : public PyType {

public:
TupleType(PyObject *obj);
const TYPE returnType = TYPE::TUPLE;

/**
 * @brief Gets the tuple item at the given index
 *
 * @param index The index of the item in question
 * @return PyType* Returns a pointer to the appropriate PyType object
 */
PyType *get(int index) const;

/**
 * @brief
 *
 *
 *
 * @returns int length of the tuple
 */
int len() const;

/**
 * @brief Helper function for print()
 *
 * @param os output stream to print to
 * @param depth depth into sub-objects
 */
void print_helper(std::ostream &os, int depth = 0) const;

protected:
/**
 * @brief Override to the print method defined in PyType to enable us to print this class easily
 *
 * @param os output stream to print to
 */
virtual void print(std::ostream &os) const override;
};
#endif