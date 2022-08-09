#include <string>
#include "../include/IntType.hpp"

IntType::IntType(PyObject* object): PyType(object) {
}

IntType::IntType(long n): PyType(Py_BuildValue("i", n)) {
}

void IntType::print(std::ostream& os) const {
    os << this->getValue();
}

long IntType::getValue() const {
    return PyLong_AS_LONG(pyObject);
}

std::string IntType::getReturnType() {
    return this->returnType;
}