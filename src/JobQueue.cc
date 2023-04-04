#include "include/JobQueue.hh"
#include "include/pyTypeFactory.hh"

#include <Python.h>

#include <jsfriendapi.h>

JSObject *JobQueue::getIncumbentGlobal(JSContext *cx) {
  return JS::CurrentGlobalOrNull(cx);
}

bool JobQueue::enqueuePromiseJob(JSContext *cx,
  [[maybe_unused]] JS::HandleObject promise,
  JS::HandleObject job,
  [[maybe_unused]] JS::HandleObject allocationSite,
  [[maybe_unused]] JS::HandleObject incumbentGlobal) {

  // Convert the `job` JS function to a Python function for event-loop callback
  // TODO (Tom Tang): assert `job` is JS::Handle<JSFunction*> by JS::GetBuiltinClass(...) == js::ESClass::Function (17)
  // FIXME (Tom Tang): objects not free-ed
  auto global = new JS::RootedObject(cx, getIncumbentGlobal(cx));
  auto jobv = new JS::RootedValue(cx, JS::ObjectValue(*job.get()));
  auto callback = pyTypeFactory(cx, global, jobv)->getPyObject();

  // Get the running Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.get_running_loop
  PyObject *asyncio = PyImport_ImportModule("asyncio");
  PyObject *loop = PyObject_CallMethod(asyncio, "get_running_loop", NULL);
  if (loop == nullptr) { // `get_running_loop` would raise a RuntimeError if there is no running event loop
    // PyErr_SetString(PyExc_RuntimeError, "No running Python event-loop."); // override the error raised by `get_running_loop`
    Py_DECREF(asyncio);
    return false;
  }

  // Enqueue job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_soon
  auto methodName = PyUnicode_DecodeFSDefault("call_soon");
  auto asyncHandle = PyObject_CallMethodOneArg(loop, methodName, callback);
  // TODO (Tom Tang): refactor python calls into its own method

  // Inform the JS runtime that the job queue is no longer empty
  JS::JobQueueMayNotBeEmpty(cx);

  // Clean up
  Py_DECREF(asyncio);
  Py_DECREF(loop);
  Py_DECREF(methodName);
  Py_DECREF(asyncHandle);

  return true;
}

void JobQueue::runJobs(JSContext *cx) {
  // TODO (Tom Tang):
  printf("JobQueue::runJobs is unimplemented\n");
  return;
}

// is empty
bool JobQueue::empty() const {
  // TODO (Tom Tang): implement using `get_running_loop` and getting job count on loop???
  printf("JobQueue::empty is unimplemented\n");
  return false;
}

js::UniquePtr<JS::JobQueue::SavedJobQueue> JobQueue::saveJobQueue(JSContext *cx) {
  // TODO (Tom Tang): implement this method way later
  printf("JobQueue::saveJobQueue is unimplemented\n");
  return nullptr;
}

bool JobQueue::init(JSContext *cx) {
  JS::SetJobQueue(cx, this);
  // TODO (Tom Tang): JS::InitDispatchToEventLoop(...)
  //       see inside js::UseInternalJobQueues(cx); (initInternalDispatchQueue)
  return true;
}
