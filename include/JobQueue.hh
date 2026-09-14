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
 * a JS microtask (LOCAL PATCH: new pure-virtual method added alongside the
 * SpiderMonkey 157a1 JobQueue redesign -- see runJobs() below for context).
 *
 * Mirrors the "we don't track this" stance already taken in
 * getHostDefinedData() above: we have no host defined global of our own, so
 * SpiderMonkey falls back to its own default (the microtask's execution
 * global, from GetExecutionGlobalFromJSMicroTask). Matches SpiderMonkey's
 * own reference embedding, InternalJobQueue::getHostDefinedGlobal, which
 * does exactly this (js/src/vm/JSContext.cpp).
 */
bool getHostDefinedGlobal(JSContext *cx, JS::MutableHandle<JSObject *> out) const override;

/**
 * @brief Pull every job SpiderMonkey has queued internally since the last
 * call, and forward each one to the Python event-loop for execution.
 *
 * LOCAL PATCH (SpiderMonkey 157a1 API change): `JobQueue::enqueuePromiseJob`
 * -- the old per-job push callback this class used to override -- was
 * removed from the base class entirely. SpiderMonkey now enqueues promise
 * reaction jobs into its own internal queue as it creates them (see
 * EnqueueJob() in js/src/builtin/Promise.cpp), without notifying the
 * embedding. The embedding is instead expected to pull queued jobs itself,
 * here, whenever it wants a "microtask checkpoint" to happen -- triggered
 * by the embedder calling the free function js::RunJobs(cx) (declared in
 * jsfriendapi.h; NOT the same thing as this method, despite the identical
 * name -- js::RunJobs(cx) is what calls cx->jobQueue->runJobs(cx), i.e.
 * this override). PythonMonkey calls js::RunJobs(GLOBAL_CX) once after each
 * top-level JS_ExecuteScript() call, in pythonmonkey.cc.
 *
 * This preserves the original behaviour -- JS promise reactions execute as
 * Python asyncio callbacks, not synchronously inline -- by draining
 * SpiderMonkey's internal queue and re-creating the same "hand this job to
 * Python's event loop" forwarding enqueuePromiseJob used to do per-job, just
 * done here in a pull/batch fashion instead.
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
 * LOCAL PATCH (SpiderMonkey 157a1 API change): `JS::InitDispatchToEventLoop`
 * (2-callback init) was replaced by `JS::InitAsyncTaskCallbacks`, which now
 * mandates both a `DispatchToEventLoopCallback` AND a
 * `DelayedDispatchToEventLoopCallback` (see delayedDispatchToEventLoop()
 * below). The callback signature itself also changed: it now takes ownership
 * of the Dispatchable via `js::UniquePtr<Dispatchable>&&` instead of a raw
 * pointer, and `Dispatchable::run()` is now `protected` -- callers must go
 * through the new public static `Dispatchable::Run(cx, task, shuttingDown)`
 * instead of calling `->run()` directly.
 *
 * @param closure - closure, currently the javascript context
 * @param dispatchable - the Dispatchable to be called; ownership transferred to this callback
 * @return not shutting down
 */
static bool dispatchToEventLoop(void *closure, js::UniquePtr<JS::Dispatchable> &&dispatchable);

/**
 * @brief The callback for dispatching an off-thread promise to the event
 * loop after a delay (LOCAL PATCH: newly mandatory as of the same API
 * change described on dispatchToEventLoop() above -- previously this
 * concept didn't need to exist as a separate callback for this embedding).
 *
 * NEEDS REVIEW: this embedding has no cross-thread-safe delayed-dispatch
 * mechanism (PyEventLoop::enqueueWithDelay exists but calls
 * asyncio.loop.call_later, which -- unlike call_soon_threadsafe, used
 * elsewhere in this codebase -- is not documented as safe to call from a
 * thread other than the one running the loop; this callback, per its
 * declaration in js/public/Promise.h, must be safe to call from ANY
 * thread). Per that same header's documented contract ("If a timeout
 * manager is not available for given context, it should return false"),
 * this always returns false, i.e. this embedding declines to service
 * engine-level delayed dispatch. This should only affect internal
 * SpiderMonkey features that specifically need a delayed off-thread
 * callback (e.g. an Atomics.waitAsync timeout) -- ordinary JS
 * `setTimeout`/`setInterval` in pythonmonkey go through a separate,
 * already-working path (PyEventLoop::enqueueWithDelay called from JS-exposed
 * timer functions, not this SpiderMonkey-internal callback) and are
 * unaffected. Not verified against a real Atomics.waitAsync-with-timeout
 * test case.
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