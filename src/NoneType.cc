#include "include/NoneType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

NoneType::NoneType() : PyType(Py_None) {}