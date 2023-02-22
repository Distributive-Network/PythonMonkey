/**
 * @file NoneType.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing None
 * @version 0.1
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/NoneType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <iostream>


NoneType::NoneType() : PyType(Py_None) {}

void NoneType::print(std::ostream &os) const {
  os << "None";
}