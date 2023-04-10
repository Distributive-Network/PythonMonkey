#include "include/JobQueue.hh"
#include "include/PyEventLoop.hh"
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
  JS::HandleObject incumbentGlobal) {

  // Convert the `job` JS function to a Python function for event-loop callback
  // TODO (Tom Tang): assert `job` is JS::Handle<JSFunction*> by JS::GetBuiltinClass(...) == js::ESClass::Function (17)
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
  JS::InitDispatchToEventLoop(cx, /* callback */ dispatchToEventLoop, /* closure */ cx);
  return true;
}

/* static */
bool JobQueue::dispatchToEventLoop(void *closure, JS::Dispatchable *dispatchable) {
  JSContext *cx = (JSContext *)closure; // `closure` is provided in `JS::InitDispatchToEventLoop` call
  // dispatchable->run(cx, JS::Dispatchable::NotShuttingDown);
  return true;
}
