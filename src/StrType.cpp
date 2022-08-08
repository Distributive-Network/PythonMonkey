#include "../include/StrType.hpp"

StrType::StrType(PyObject* object): PyType(object) {
}

StrType::StrType(char* string): PyType(Py_BuildValue("s", string)) {
}

void StrType::print(std::ostream& os) const {
  os << this->getValue();
}

const char* StrType::getValue() const {
  return PyUnicode_AsUTF8(pyObject);
}