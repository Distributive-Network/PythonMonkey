/**
 * @file JobQueue.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief Implements the ECMAScript Job Queue
 * @date 2023-04-03
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JobQueue_
#define PythonMonkey_JobQueue_

#include <jsapi.h>
#include <js/Promise.h>

#include <Python.h>

/**
 * @brief Implement the ECMAScript Job Queue:
 * https://www.ecma-international.org/ecma-262/9.0/index.html#sec-jobs-and-job-queues
 * @see https://hg.mozilla.org/releases/mozilla-esr102/file/5741ffa/js/public/Promise.h#l22
 */
class JobQueue : public JS::JobQueue {

public:
explicit JobQueue(JSContext *cx);
~JobQueue() = default;

/**
 * @brief Initialize PythonMonkey's event-loop job queue
 * @param cx - javascript context pointer
 * @return success
 */
bool init(JSContext *cx);

/**
 * @brief Ask the embedding for the incumbent global.
 *
 * SpiderMonkey doesn't itself have a notion of incumbent globals as defined
 * by the HTML spec, so we need the embedding to provide this. See
 * dom/script/ScriptSettings.h for details.
 */
JSObject *getIncumbentGlobal(JSContext *cx) override;

/**
 * @brief Enqueue a reaction job `job` for `promise`, which was allocated at
 * `allocationSite`. Provide `incumbentGlobal` as the incumbent global for
 * the reaction job's execution.
 *
 * `promise` can be null if the promise is optimized out.
 * `promise` is guaranteed not to be optimized out if the promise has
 * non-default user-interaction flag.
 */
bool enqueuePromiseJob(JSContext *cx, JS::HandleObject promise,
  JS::HandleObject job, JS::HandleObject allocationSite,
  JS::HandleObject incumbentGlobal) override;

/**
 * @brief Run all jobs in the queue. Running one job may enqueue others; continue to
 * run jobs until the queue is empty.
 *
 * Calling this method at the wrong time can break the web. The HTML spec
 * indicates exactly when the job queue should be drained (in HTML jargon,
 * when it should "perform a microtask checkpoint"), and doing so at other
 * times can incompatibly change the semantics of programs that use promises
 * or other microtask-based features.
 *
 * This method is called only via AutoDebuggerJobQueueInterruption, used by
 * the Debugger API implementation to ensure that the debuggee's job queue is
 * protected from the debugger's own activity. See the comments on
 * AutoDebuggerJobQueueInterruption.
 */
void runJobs(JSContext *cx) override;

/**
 * @return true if the job queue is empty, false otherwise.
 */
bool empty() const override;

/**
 * @return true if the job queue stopped draining, which results in `empty()` being false after `runJobs()`.
 */
bool isDrainingStopped() const override;

/**
 * @brief Appends a callback to the queue of FinalizationRegistry callbacks
 *
 * @param callback - the callback to be queue'd
 */
void queueFinalizationRegistryCallback(JSFunction *callback);

/**
 * @brief Runs the accumulated queue of FinalizationRegistry callbacks
 *
 * @param cx - Pointer to the JSContext
 * @return true - at least 1 callback was called
 * @return false - no callbacks were called
 */
bool runFinalizationRegistryCallbacks(JSContext *cx);

private:

using FunctionVector = JS::GCVector<JSFunction *, 0, js::SystemAllocPolicy>;
JS::PersistentRooted<FunctionVector> *finalizationRegistryCallbacks;

/**
 * @brief Capture this JobQueue's current job queue as a SavedJobQueue and return it,
 * leaving the JobQueue's job queue empty. Destroying the returned object
 * should assert that this JobQueue's current job queue is empty, and restore
 * the original queue.
 *
 * On OOM, this should call JS_ReportOutOfMemory on the given JSContext,
 * and return a null UniquePtr.
 */
js::UniquePtr<JS::JobQueue::SavedJobQueue> saveJobQueue(JSContext *) override;

/**
 * @brief The callback for dispatching an off-thread promise to the event loop
 *          see https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/public/Promise.h#l580
 *              https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/OffThreadPromiseRuntimeState.cpp#l160
 * @param closure - closure, currently the javascript context
 * @param dispatchable - Pointer to the Dispatchable to be called
 * @return not shutting down
 */
static bool dispatchToEventLoop(void *closure, JS::Dispatchable *dispatchable);

/**
 * @brief The callback that gets invoked whenever a Promise is rejected without a rejection handler (uncaught/unhandled exception)
 *          see https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/public/Promise.h#l268
 *              https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/Runtime.cpp#l600
 * @param promise - The Promise object
 * @param state - Is the Promise unhandled?
 * @param mutedErrors - When the `mutedErrors` option in `pm.eval` is set to true, unhandled rejections are ignored ("muted").
 *                      See also https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/public/CompileOptions.h#l129
 * @param privateData - unused
 */
static void promiseRejectionTracker(JSContext *cx, bool mutedErrors,
  JS::HandleObject promise, JS::PromiseRejectionHandlingState state,
  void *privateData);

}; // class

/**
 * @brief Send job to the Python event-loop on main thread
 * (Thread-Safe)
 * @param pyFunc - the Python job function
 * @return success
 */
bool sendJobToMainLoop(PyObject *pyFunc);

#endif