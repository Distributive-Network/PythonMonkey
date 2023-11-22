#include "include/ListType.hh"

#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"

#include <Python.h>

#include <js/Array.h>
#include <js/ValueArray.h>


ListType::ListType() : PyType(PyList_New(0)) {}

ListType::ListType(PyObject *object) : PyType(object) {}

ListType::ListType(JSContext *cx, JS::HandleObject arrayObj) {
  uint32_t length;
  JS::GetArrayLength(cx, arrayObj, &length);

  PyObject *object = PyList_New((Py_ssize_t)length);
  Py_XINCREF(object);
  this->pyObject = object;

  JS::RootedValue rval(cx);
  JS::RootedObject arrayRootedObj(cx, arrayObj);

  for (uint32_t index = 0; index < length; index++) {
    JS_GetElement(cx, arrayObj, index, &rval);

    PyType *pyType = pyTypeFactory(cx, &arrayRootedObj, &rval);

    PyList_SetItem(this->pyObject, index, pyType->getPyObject());
  }
}