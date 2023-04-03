#include "include/JobQueue.hh"

#include <jsfriendapi.h>

JSObject *JobQueue::getIncumbentGlobal(JSContext *cx) {
  return JS::CurrentGlobalOrNull(cx);
}

bool JobQueue::enqueuePromiseJob(JSContext *cx,
  [[maybe_unused]] JS::HandleObject promise,
  JS::HandleObject job,
  [[maybe_unused]] JS::HandleObject allocationSite,
  [[maybe_unused]] JS::HandleObject incumbentGlobal) {
  printf("JobQueue::enqueuePromiseJob is unimplemented\n");
  return true;
}

void JobQueue::runJobs(JSContext *cx) {
  printf("JobQueue::runJobs is unimplemented\n");
  return;
}

// is empty
bool JobQueue::empty() const {
  printf("JobQueue::empty is unimplemented\n");
  return false;
}

js::UniquePtr<JS::JobQueue::SavedJobQueue> JobQueue::saveJobQueue(JSContext *cx) {
  // TODO (Tom Tang): implement this method
  printf("JobQueue::saveJobQueue is unimplemented\n");
  return nullptr;
}

bool JobQueue::init(JSContext *cx) {
  JS::SetJobQueue(cx, this);
  // return js::UseInternalJobQueues(cx);
  return true;
}
