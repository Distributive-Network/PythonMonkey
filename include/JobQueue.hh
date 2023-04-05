/**
 * @file JobQueue.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief
 * @version 0.1
 * @date 2023-04-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PythonMonkey_JobQueue_
#define PythonMonkey_JobQueue_

#include <jsapi.h>
#include <js/Promise.h>

#include <Python.h>

class JobQueue : public JS::JobQueue {
//
// JS::JobQueue methods.
//    see also: js::InternalJobQueue https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/JSContext.h#l88
//
public:
~JobQueue() = default;

JSObject *getIncumbentGlobal(JSContext *cx) override;

bool enqueuePromiseJob(JSContext *cx, JS::HandleObject promise,
  JS::HandleObject job, JS::HandleObject allocationSite,
  JS::HandleObject incumbentGlobal) override;

void runJobs(JSContext *cx) override;

/**
 * @return true if the job queue is empty, false otherwise.
 */
bool empty() const override;

private:
js::UniquePtr<JS::JobQueue::SavedJobQueue> saveJobQueue(JSContext *) override;

//
// Custom methods
//
public:
/**
 * @brief Initialize PythonMonkey's event-loop job queue
 * @param cx - javascript context pointer
 * @return success
 */
bool init(JSContext *cx);

private:
/**
 * @brief The callback for dispatching an off-thread promise to the event loop
 *          see https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/public/Promise.h#l580
 *              https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/OffThreadPromiseRuntimeState.cpp#l160
 * @param closure - closure, currently the javascript context
 * @param dispatchable - Pointer to the Dispatchable to be called
 * @return not shutting down
 */
static bool dispatchToEventLoop(void *closure, JS::Dispatchable *dispatchable);
};

#endif