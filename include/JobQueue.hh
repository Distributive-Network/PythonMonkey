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
 * @brief Ask the embedding for the host defined data.
 *
 * SpiderMonkey doesn't itself have a notion of host defined data as defined
 * by the HTML spec, so we need the embedding to provide this. See
 * dom/script/ScriptSettings.h for details.
 *
 * If the embedding has the host defined data, this method should return the
 * host defined data via the `data` out parameter and return `true`.
 * The object in the `data` out parameter can belong to any compartment.
 * If the embedding doesn't need the host defined data, this method should
 * set the `data` out parameter to `nullptr` and return `true`.
 * If any error happens while generating the host defined data, this method
 * should set a pending exception to `cx` and return `false`.
 */
bool getHostDefinedData(JSContext *cx, JS::MutableHandle<JSObject *> incumbentGlobal, JS::MutableHandle<JSObject *> data) const override;

/**
 * @brief Ask the embedding for the host defined global to use when running
 * a JS microtask.
 *
 * Same "we don't track this" stance as getHostDefinedData() above -- falls
 * back to SpiderMonkey's own default, matching the reference embedding
 * (InternalJobQueue::getHostDefinedGlobal, js/src/vm/JSContext.cpp).
 */
bool getHostDefinedGlobal(JSContext *cx, JS::MutableHandle<JSObject *> out) const override;

/**
 * @brief Pull every job SpiderMonkey has queued internally since the last
 * call, and forward each one to the Python event-loop for execution.
 *
 * SpiderMonkey no longer pushes promise jobs to the embedding as they're
 * created (the old enqueuePromiseJob); it queues them internally and
 * expects the embedder to pull them here on demand, via the free function
 * js::RunJobs(cx) (jsfriendapi.h -- not the same thing as this method: it's
 * what calls cx->jobQueue->runJobs(cx)). PythonMonkey calls
 * js::RunJobs(GLOBAL_CX) after every top-level JS_ExecuteScript(), plus a
 * few call sites where JS callbacks resolve promises outside of script
 * execution (see JSFunctionProxy.cc, PromiseType.cc).
 *
 * Calling this method at the wrong time can break the web. The HTML spec
 * indicates exactly when the job queue should be drained (in HTML jargon,
 * when it should "perform a microtask checkpoint"), and doing so at other
 * times can incompatibly change the semantics of programs that use promises
 * or other microtask-based features.
 */
void runJobs(JSContext *cx) override;

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
 *
 * Takes ownership of the Dispatchable (run via the public static
 * Dispatchable::Run, since Dispatchable::run() is protected).
 *
 * @param closure - closure, currently the javascript context
 * @param dispatchable - the Dispatchable to be called; ownership transferred to this callback
 * @return not shutting down
 */
static bool dispatchToEventLoop(void *closure, js::UniquePtr<JS::Dispatchable> &&dispatchable);

/**
 * @brief The callback for dispatching an off-thread promise to the event
 * loop after a delay.
 *
 * Always returns false (no timeout manager available), which
 * js/public/Promise.h documents as a valid response when the embedding
 * can't service delayed cross-thread dispatch. Only affects SpiderMonkey
 * features needing a delayed off-thread callback (e.g. an
 * Atomics.waitAsync timeout) -- ordinary setTimeout/setInterval go through
 * PyEventLoop::enqueueWithDelay instead and are unaffected. NEEDS REVIEW:
 * not verified against a real Atomics.waitAsync-with-timeout case.
 *
 * @param closure - closure, currently the javascript context
 * @param dispatchable - the Dispatchable that would be called; ownership transferred to this callback
 * @param delay - requested delay in milliseconds
 * @return false (no timeout manager available for cross-thread delayed dispatch)
 */
static bool delayedDispatchToEventLoop(void *closure, js::UniquePtr<JS::Dispatchable> &&dispatchable, uint32_t delay);

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