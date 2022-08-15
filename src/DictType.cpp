#include <include/DictType.hpp>
#include <Python.h>
#include <optional>

#include <include/TypeEnum.hpp>
#include <include/PyTypeFactory.hpp>

DictType::DictType(PyObject* object): PyType(object) {
}

void DictType::print(std::ostream& os) const {
    PyObject* keys = PyDict_Keys(this->pyObject);

    const Py_ssize_t keys_size = PyList_Size(keys);

    os << "{\n";
    for(int i = 0; i < keys_size; i++) {
        PyType* key = PyTypeFactory(PyList_GetItem(keys, i));
        PyType* value = this->get(key).value();

        os << "  " << *key << ":" << *value;
        if(i < keys_size - 1) {
            os << ",\n";
        }
    }
    os << "\n}";

}

// NOTE: Maybe this should return something on success/failure?
void DictType::set(PyType* key, PyType* value) {
    PyDict_SetItem(this->pyObject, key->getPyObject(), value->getPyObject());
}

// NOTE: This could possible return a std::optional if the item does not exist
std::optional<PyType*> DictType::get(PyType* key) const { 
    PyObject* retrieved_object = PyDict_GetItem(this->pyObject, key->getPyObject());

    return retrieved_object != NULL ? std::optional<PyType*>{PyTypeFactory(retrieved_object)} : std::nullopt;
}

