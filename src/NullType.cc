/**
 * @file NullType.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing JS null in a python object
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/NullType.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/PyType.hh"
#include "include/TypeEnum.hh"

NullType::NullType() : PyType(PythonMonkey_Null) {}