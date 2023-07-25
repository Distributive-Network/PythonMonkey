
#include "include/JobQueue.hh"

#include "include/PyEventLoop.hh"
#include "include/pyTypeFactory.hh"

#include <Python.h>
#include <jsfriendapi.h>

#include <stdexcept>

JSObject *JobQueue::getIncumbentGlobal(JSContext *cx) {
  return JS::CurrentGlobalOrNull(cx);
}

bool JobQueue::enqueuePromiseJob(JSContext *cx,
  [[maybe_unused]] JS::HandleObject promise,
  JS::HandleObject job,
  [[maybe_unused]] JS::HandleObject allocationSite,
  JS::HandleObject incumbentGlobal) {

  // Convert the `job` JS function to a Python function for event-loop callback
  MOZ_RELEASE_ASSERT(js::IsFunctionObject(job));
  // FIXME (Tom Tang): memory leak, objects not free-ed
  // FIXME (Tom Tang): `job` function is going to be GC-ed ???
  auto global = new JS::RootedObject(cx, incumbentGlobal);
  auto jobv = new JS::RootedValue(cx, JS::ObjectValue(*job));
  auto callback = pyTypeFactory(cx, global, jobv)->getPyObject();

  // Inform the JS runtime that the job queue is no longer empty
  JS::JobQueueMayNotBeEmpty(cx);

  // Send job to the running Python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return false;
  loop.enqueue(callback);

  return true;
}

void JobQueue::runJobs(JSContext *cx) {
  return;
}

// is empty
bool JobQueue::empty() const {
  // TODO (Tom Tang): implement using `get_running_loop` and getting job count on loop???
  throw std::logic_error("JobQueue::empty is not implemented\n");
}

js::UniquePtr<JS::JobQueue::SavedJobQueue> JobQueue::saveJobQueue(JSContext *cx) {
  auto saved = js::MakeUnique<JS::JobQueue::SavedJobQueue>();
  return saved;
}

bool JobQueue::init(JSContext *cx) {
  JS::SetJobQueue(cx, this);
  JS::InitDispatchToEventLoop(cx, /* callback */ dispatchToEventLoop, /* closure */ cx);
  return true;
}

static PyObject *callDispatchFunc(PyObject *dispatchFuncTuple, PyObject *Py_UNUSED(unused)) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 0));
  JS::Dispatchable *dispatchable = (JS::Dispatchable *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 1));
  dispatchable->run(cx, JS::Dispatchable::NotShuttingDown);
  Py_RETURN_NONE;
}
static PyMethodDef callDispatchFuncDef = {"JsDispatchCallable", callDispatchFunc, METH_NOARGS, NULL};

bool sendJobToMainLoop(PyObject *pyFunc) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  // Send job to the running Python event-loop on `cx`'s thread (the main thread)
  PyEventLoop loop = PyEventLoop::getMainLoop();
  if (!loop.initialized()) {
    PyGILState_Release(gstate);
    return false;
  }
  loop.enqueue(pyFunc);

  PyGILState_Release(gstate);
  return true;
}

/* static */
bool JobQueue::dispatchToEventLoop(void *closure, JS::Dispatchable *dispatchable) {
  JSContext *cx = (JSContext *)closure; // `closure` is provided in `JS::InitDispatchToEventLoop` call

  // The `dispatchToEventLoop` function is running in a helper thread, so
  // we must acquire the Python GIL (global interpreter lock)
  //    see https://docs.python.org/3/c-api/init.html#non-python-created-threads
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject *dispatchFuncTuple = PyTuple_Pack(2, PyLong_FromVoidPtr(cx), PyLong_FromVoidPtr(dispatchable));
  PyObject *pyFunc = PyCFunction_New(&callDispatchFuncDef, dispatchFuncTuple);

  // Avoid using the JS helper thread to send jobs to event-loop as it may cause deadlock
  PyThread_start_new_thread((void (*)(void *)) &sendJobToMainLoop, pyFunc);

  PyGILState_Release(gstate);
  return true; // dispatchable must eventually run
}
