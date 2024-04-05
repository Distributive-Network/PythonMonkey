/**
 * @file NoneType.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing None
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/NoneType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

NoneType::NoneType() : PyType(Py_None) {}