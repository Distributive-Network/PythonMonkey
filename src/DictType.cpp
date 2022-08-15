#include <include/DictType.hpp>
#include <Python.h>
#include <optional>

#include <include/TypeEnum.hpp>
#include <include/PyTypeFactory.hpp>
#include <include/utilities.hpp>

DictType::DictType(PyObject* object): PyType(object) {
}

void DictType::print(std::ostream& os) const {
    print_helper(os);
}

void DictType::print_helper(std::ostream& os, int depth) const {
    PyObject* keys = PyDict_Keys(this->pyObject);

    const Py_ssize_t keys_size = PyList_Size(keys);


    os << "{\n  ";
    for(int i = 0; i < keys_size; i++) {
        PyType* key = PyTypeFactory(PyList_GetItem(keys, i));
        PyType* value = this->get(key).value();

        if(instanceof<DictType>(value)) {
            DictType* casted_value = dynamic_cast<DictType*>(value);
            os << *key << ":";
            casted_value->print_helper(os, depth + 1);
        } else {
            os << std::string(depth * 2, ' ') << *key << ":" << *value;
        }
        if(i < keys_size - 1) {
            os << ",\n  ";
        }
    }
    os << std::endl << std::string(depth * 2, ' ') << "}";
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

