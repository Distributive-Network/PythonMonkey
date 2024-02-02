#include "include/NullType.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/PyType.hh"
#include "include/TypeEnum.hh"

NullType::NullType() : PyType(PythonMonkey_Null) {}