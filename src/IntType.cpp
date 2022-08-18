#include <string>
#include "include/IntType.hpp"
#include "include/TypeEnum.hpp"

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

TYPE IntType::getReturnType() const {
    return this->returnType;
}

PyObject* IntType::factor() const {
    PyObject* list = PyList_New(0);
    Py_XINCREF(list);

    double sqrt_value = sqrt(this->getValue());


    for(int i = 1; i < sqrt_value; i++) {
        if(this->getValue() % i == 0) {
            PyObject* append_value_1 = Py_BuildValue("i", i);
            PyObject* append_value_2 = Py_BuildValue("i", this->getValue()/i);
            PyList_Append(list, append_value_1);
            PyList_Append(list, append_value_2);
        }
    }
    PyList_Sort(list);

    Py_XDECREF(list); 
    return list;
}
