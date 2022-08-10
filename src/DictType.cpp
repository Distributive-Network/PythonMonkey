#include <include/DictType.hpp>
#include <Python.h>
#include <optional>

#include <include/TypeEnum.hpp>
#include <include/PyTypeFactory.hpp>

DictType::DictType(PyObject* object): PyType(object) {
}

void DictType::print(std::ostream& os) const {
    os << "something for now";
}

// NOTE: Maybe this should return something on success/failure?
void DictType::set(PyType* key, PyType* value) {
    PyDict_SetItem(this->pyObject, key->getPyObject(), value->getPyObject());
}

// NOTE: This could possible return a std::optional if the item does not exist
PyType* DictType::get(PyType* key) { 
    PyObject* retrieved_object = PyDict_GetItemWithError(this->pyObject, key->getPyObject());

    return retrieved_object != NULL ? PyTypeFactory(retrieved_object) : NULL;
}