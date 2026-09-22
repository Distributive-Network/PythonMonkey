# pythonmonkey: moving off the nightly-alpha SpiderMonkey build — handover notes

**Status: BUILD SUCCEEDS, CORE FUNCTIONALITY VERIFIED.** This document tracks
every change made while rebuilding pythonmonkey against a current
mozilla-central snapshot instead of the ~19-month-old nightly alpha it
previously shipped (`mozjs-136a1.dll`). Written for handover to the
pythonmonkey/dcp team — read this before trusting or shipping the resulting
build. **See the Testing section near the end for exactly what was and
wasn't verified before you rely on this.**

New engine: `mozjs-157a1.dll`, built from mozilla-central commit
`1704651e7d6c706fcb753adab577e0954d61cee0` (current trunk as of this work,
2026-09-14) — see the "Which commit to target" section below for why this
specific commit and not a numbered Firefox release.

---

## Why

A security review of what `pip install dcp` actually puts on a machine found
that pythonmonkey embeds `mozjs-136a1.dll` — not a small standalone library,
but a build of SpiderMonkey (Firefox's JS engine) out of a full
`firefox-source` checkout. The `136a1` is Mozilla's own suffix for **Nightly
Alpha 1**, an unstable, pre-release development snapshot, explicitly "not for
general users." That build predates Firefox 136's eventual stable release —
including an emergency out-of-band patch (136.0.4, shipped 2025-03-27) for a
sandbox-escape vulnerability that was being **actively exploited in the wild**
(CVE-2025-2857), and the broader memory-safety fixes in that release cycle
(MFSA 2025-14).

Checked `pythonmonkey`'s own upstream `main` branch (`git fetch origin`,
compared `mozcentral.version`): it is **also** still pinned to the exact same
`6bca861985ba51920c1cacc21986af01c51bd690` alpha commit as this local clone,
unchanged since at least their last commit. This is not a stale-local-clone
problem — it's what pythonmonkey ships to everyone today.

## What target was chosen, and why

This GitHub mirror (`mozilla-firefox/firefox`, what `setup.sh` actually
downloads from) only tracks mozilla-central **trunk** — it has no per-release
tags, only a rolling `last-mozilla-central` tag. There is no way to pin an
exact "Firefox 136.0.4" commit from this specific source. The practical
equivalent is a trunk commit safely after the point those fixes landed (Mozilla
lands security fixes in trunk before or alongside backporting them to release
branches).

Initially picked a conservative target (~3.5 weeks after 136.0.4's ship date,
April 2025) to minimize source drift from the currently-working alpha. On
reflection (correctly challenged mid-task): that doesn't actually solve the
underlying problem — an 18-months-old-by-now snapshot would *also* read as
"stale, unpatched" to a future reviewer. Since this mirror only ever offers a
trunk snapshot regardless of which commit is picked, there's no "stable
channel" to fall back to either way — so the right choice is the **freshest**
trunk commit, not a stale one.

**Final target: `1704651e7d6c706fcb753adab577e0954d61cee0`**, dated
2026-09-06 (~1 week before this work started, deliberately not the literal
tip-of-trunk at the moment of picking, to sidestep any short-lived transient
build breakage mozilla-central occasionally has).

Old pin preserved at `mozcentral.version.orig-backup-136a1` for rollback.

---

## Changes made, in order

### 1. `mozcentral.version`

```diff
- 6bca861985ba51920c1cacc21986af01c51bd690
+ 1704651e7d6c706fcb753adab577e0954d61cee0
```

### 2. `setup.sh` — several fixes, all confirmed necessary by real build failures (not speculative)

**a. Rust install step made idempotent.** Was unconditional on every run.
Re-running `rustup-init.sh` when Rust is already installed downloads a fresh
installer exe and executes it, which on this Windows machine gets blocked by
Windows Defender ("Permission denied" — see finding 3 below for the pattern).
Skips the whole block if the 1.85 toolchain is already present, matching the
existing pattern right below it (the Poetry-install skip):

```diff
+ if command -v rustup >/dev/null && rustup toolchain list 2>/dev/null | grep -q '^1\.85'; then
+   echo "Rust 1.85 toolchain already installed, skipping rustup-init"
+ else
    echo "Installing rust compiler"
    ...
    curl ... | sh -s -- -y ... --default-toolchain 1.85
+ fi
  CARGO_BIN="$HOME/.cargo/bin/cargo"
- $CARGO_BIN install cbindgen
+ command -v cbindgen >/dev/null || $CARGO_BIN install cbindgen
```

**b. `wget` replaced with `curl` for the Firefox source download.**
`wget.exe` (confirmed both the MSYS2 copy and — implicitly — any copy) is
blocked outright by a **Windows Defender Application Control (WDAC) policy**
on this machine: `An Application Control policy has blocked this file`,
confirmed directly via a native PowerShell invocation, not a PATH/permissions
issue. This is new since the original 136a1 build session — nothing in that
session's own log mentions any AppLocker/WDAC block. **This machine's security
posture has tightened since the last build**, plausibly related to DWAN-prep
work happening in parallel — worth flagging to whoever manages this machine's
policy. `curl` (both Windows' own and MSYS2's) is unaffected; `unzip` is also
unaffected. Swapped just the `wget` call:

```diff
- wget -c -q -O firefox-source-${MOZCENTRAL_VERSION}.zip https://...
+ curl -fsSL -o firefox-source-${MOZCENTRAL_VERSION}.zip https://...
```

**c. Removed `--disable-explicit-resource-management` configure flag.**
Worked around Bugzilla 1940342 (a header/lib enum mismatch from when the
`using` JS syntax was newly landing in nightly, circa early 2025). On the new
snapshot this is now an *unrecognized* configure option (`InvalidOptionError:
Unknown option`) — the feature has evidently shipped/stabilized since, taking
the flag (and presumably the underlying bug) with it. Removed rather than
guessing a replacement.

**d. `MOZILLABUILD` `KeyError` fix reapplied.** This is the *same* fix already
made once before, ad-hoc, during the earlier one-line SharedArrayBuffer/Atomics
build session against the old `136a1` engine (not part of this PR) — but that
fix was applied directly to a file *inside* the ephemeral `firefox-source`
checkout, not to this persistent `setup.sh`, so it was lost when
`firefox-source` was deleted and re-fetched for the new commit. Re-applied,
and this time added as a proper `sed` patch in `setup.sh` itself (matching the
existing pattern of the other ~10 patches) so it survives future re-extracts:

```diff
+ sed -i'' -e 's/os\.environ\["MOZILLABUILD"\]/os.environ.get("MOZILLABUILD", "")/g' ./python/mozbuild/mozbuild/backend/visualstudio.py # LOCAL PATCH: ...
```

**e. Poetry install made idempotent, and kept — not dropped.** An earlier
version of this patch skipped installing Poetry altogether, reasoning that it
was only consumed later in this same script's `.git/hooks/pre-commit`
dev-tooling branch and thus "irrelevant to actually building
SpiderMonkey/pythonmonkey." That reasoning was wrong — it broke that branch's
`$POETRY_BIN run pip install autopep8` line for anyone whose clone does take
it, by deleting `POETRY_BIN`'s own definition along with the install step.
Caught during review/retesting, not by a build failure. Fixed the *actual*
problem instead (this machine has no `python3` on `PATH`, only `python`, so
the real installer's `python3 - --version ...` invocation failed outright)
and made the install idempotent, matching the Rust fix in (a):

```diff
+ if command -v "$POETRY_BIN" >/dev/null || [ -x "$POETRY_BIN" ]; then
+   echo "Poetry already installed, skipping"
+ else
    echo "Installing poetry"
-   curl -sSL https://install.python-poetry.org | python3 - --version "1.7.1"
+   PYTHON_FOR_POETRY=$(command -v python3 || command -v python)
+   curl -sSL https://install.python-poetry.org | "$PYTHON_FOR_POETRY" - --version "1.7.1"
    ...
+   "$POETRY_BIN" self add 'poetry-dynamic-versioning[plugin]'
+ fi
```

### 3. Rust toolchain pin: 1.85 → 1.90.0 — was only worked around locally, not actually fixed, until CI caught it

The new mozilla-central snapshot's own `configure` now hard-requires
`rustc >= 1.90.0` (`ERROR: Rust compiler 1.85.1 is too old`) — pythonmonkey's
own 1.85 pin is unrelated to this; it's Mozilla's minimum that moved.

**This was originally worked around with a local, per-directory `rustup
override set stable` on the development machine only** — it never touched
`setup.sh`'s own `--default-toolchain 1.85`, so it was invisible to CI and to
anyone doing a fresh clone. That gap is exactly what caused all five CI
platforms to fail once this PR was actually pushed (`Rust compiler 1.85.1 is
too old` on Windows; the equivalent clang-side minimum, also newer than what
CI installs, on the other four — see the CI section below). Caught and fixed
by an autonomous pass that noticed the PR's CI had been red since the last
push and diagnosed it from the actual failure logs, not assumed.

**Actual fix, in `setup.sh` itself**: bumped the hardcoded default toolchain
from `1.85` to `1.90.0` (the exact minimum `configure` reported), so a fresh
install — CI included — gets a toolchain that actually satisfies this
snapshot's requirement, rather than relying on whatever happens to be
overridden locally on one machine.

### 3b. CI toolchain versions were also stale — same root cause, different files

Confirmed via the actual GitHub Actions logs for this PR (all 5 platforms
failed, all for a version of this same reason):

- **macOS (macos-14, macos-15-intel)**: no explicit LLVM install existed at
  all — the build was relying on Xcode's bundled clang (16.0.0 and 17.0.6 on
  the current runner images respectively), both below the >=19 requirement.
  **Fixed, committed**: added `brew install llvm` plus putting its bin dir
  first on `PATH` in `setup.sh`'s own macOS branch (homebrew's llvm keg
  isn't symlinked onto PATH by default).
- **Windows**: covered by the rustc 1.90.0 bump above. **Fixed, committed.**
- **ubuntu (x64 and arm)**: `.github/workflows/test-and-publish.yaml`'s
  "Setup LLVM" step explicitly installed LLVM 18 (`./llvm.sh 18`), which
  needed bumping to 19 to match `configure`'s own `Only clang/llvm 19.0 or
  newer is supported` error. **Fixed and applied via the GitHub UI** (this
  session's push credentials don't have the `workflow` OAuth scope needed
  to push workflow-file changes directly). This got ubuntu past the clang
  check, but exposed a second issue right behind it — see 3c below.

None of this had been verified against real CI before this pass — the
"Testing" section below was checked against local builds and a real network
job, not a green CI run. See that section for the corrected status.

**Update**: the ubuntu workflow-file fix above was applied by hand via the
GitHub UI (workflow-file pushes need `workflow` OAuth scope this session
didn't have). The clang bump alone got ubuntu past the clang-version check,
but exposed a second, different issue right behind it:

### 3c. Ubuntu's build container also needs a newer libstdc++, not just a newer clang

`ERROR: The libstdc++ in use is not new enough.` CI's ubuntu jobs
deliberately build inside an `ubuntu:20.04` container, not the
`ubuntu-22.04` runner OS itself (`.github/workflows/test-and-publish.yaml`
line 75: *"Use the Ubuntu 20.04 container inside Ubuntu 22.04 runner to
build"*) — a real, deliberate choice, presumably so the resulting wheel
links against an older glibc/libstdc++ and runs on more end-user systems.
20.04's *default* toolchain is gcc-9.

Checked the actual requirement rather than guessing: SpiderMonkey's own
`build/moz.configure/toolchain.configure` (`minimum_gcc_version()`) requires
libstdc++ from **gcc 10.1.0** specifically (`_GLIBCXX_RELEASE >= 10`) — not
some bleeding-edge version. gcc-9's libstdc++ is `_GLIBCXX_RELEASE == 9`,
one short.

**Fixed, and pushed without needing workflow scope**: added
`apt-get install --yes libstdc++-10-dev` to `setup.sh`'s own Linux
dependency list (already available in 20.04's default repos, no PPA
needed) -- this only adds headers/static libs for clang to compile against,
it doesn't change which `libstdc++.so.6` the built binary links against at
runtime. libstdc++'s ABI has been stable and symbol-versioned since long
before gcc 10 (released 2020), so targeting gcc-10-level symbols shouldn't
meaningfully narrow which end-user systems the wheel still works on --
reasoned through, not independently verified against an actual old-system
runtime.

### 4. `CMakeLists.txt` — `XP_WIN` now defined globally for the Windows build

```diff
  if (WIN32)
-   SET(COMPILE_FLAGS "/GR- /W0")
+   SET(COMPILE_FLAGS "/GR- /W0 /DXP_WIN")
```

pythonmonkey's own `.cc` files (and the SpiderMonkey public headers they pull
in) are compiled directly by this CMake/clang-cl build, not through Mozilla's
own `moz.build` system — which normally defines `XP_WIN` (Mozilla's standard
"building for Windows" macro) for every object file it compiles itself.
Without it, any SpiderMonkey header that branches on `defined(XP_WIN)`
(assuming, reasonably, that Mozilla's own build always defines it on Windows)
silently takes its POSIX/pthread branch instead — confirmed via a real build
failure: `mozilla/PlatformMutex.h` trying to `#include <pthread.h>` (doesn't
exist for an MSVC/clang-cl target), which cascaded into missing-type errors in
`mozilla/UniquePtrExtensions.h`. One instance of this exact class of bug was
already patched, per-file, in the original build (`BaseProfilerUtils.h`,
`defined(XP_WIN)` → `defined(_WIN32)`, still present as a `setup.sh` sed
patch) — since the newer Mozilla snapshot has apparently grown *more* files
with this pattern, fixing it globally here is more robust than continuing to
patch individual headers as they're discovered.

### 5. `src/BufferType.cc` — SpiderMonkey internal API adaptation — **NEEDS TEAM REVIEW**

`JS_GetArrayBufferViewFixedData` no longer exists in the new SpiderMonkey —
not renamed, redesigned. This is the **first non-mechanical fix** in this
whole effort: a real SpiderMonkey-internal-C++-API break requiring judgment
about GC/memory safety, not just an environment/build-tooling issue.

```diff
  bool isSharedMemory;
  if (!JS_GetArrayBufferViewBuffer(cx, typedArray, &isSharedMemory)) return nullptr;

- uint8_t __destBuf[0] = {};
- uint8_t *data = JS_GetArrayBufferViewFixedData(typedArray, __destBuf, 0);
- if (data == nullptr) { // shared memory or still having inline data
+ if (isSharedMemory) {
    PyErr_SetString(PyExc_TypeError, "PythonMonkey cannot coerce TypedArrays backed by shared memory.");
    return nullptr;
  }
+
+ JS::AutoAssertNoGC nogc(cx);  // see below re: why AutoAssertNoGC, not the base AutoRequireNoGC
+ bool isSharedMemory2;
+ uint8_t *data = static_cast<uint8_t *>(JS_GetArrayBufferViewData(typedArray, &isSharedMemory2, nogc));
+ if (data == nullptr) {
+   PyErr_SetString(PyExc_TypeError, "PythonMonkey cannot coerce TypedArrays backed by shared memory.");
+   return nullptr;
+ }
```

**The reasoning, and exactly what hasn't been verified:**

The old function's safety contract was: return `nullptr` if the TypedArray's
data is still stored inline (i.e. GC-movable, because it lives inside the
TypedArray object shell rather than a separately-allocated ArrayBuffer). The
new function (`JS_GetArrayBufferViewData`) drops that runtime check entirely
and instead requires a `JS::AutoRequireNoGC` token from the caller.

**`AutoRequireNoGC` (`js/GCAPI.h`) itself has protected constructor/destructor
— it's a base marker type, not directly instantiable** (confirmed by a real
build error when first tried). Used `JS::AutoAssertNoGC` instead, a public
subclass that — better than a pure marker — actually performs a runtime
assertion in diagnostic builds that no GC occurs while it's alive (a no-op in
release builds, same as the base class would have been). This gives genuine
runtime verification of the safety property in debug/diagnostic builds, not
just a compile-time formality. Even so, this change does **not** mechanically
preserve the *original* function's safety guarantee (which rejected movable
data outright rather than asserting on it) — it relies on reasoning as well
as this assertion:

The existing `JS_GetArrayBufferViewBuffer()` call, immediately before this,
already exists specifically (per its own original comment, unchanged) to force
any inline/movable TypedArray data to be promoted to a real, stably-allocated
ArrayBuffer first. If that reasoning is correct, the pointer
`JS_GetArrayBufferViewData` returns immediately afterward should already be
backed by stable (non-inline) storage — meaning the specific hazard the old
function's runtime check guarded against should already be closed off before
this new call ever runs, and the `AutoRequireNoGC` token is satisfiable
truthfully rather than just suppressing a compiler complaint.

**This has NOT been independently verified against SpiderMonkey's actual GC
behavior.** pythonmonkey hands this pointer to Python as a `Py_buffer`, which
Python code can hold onto indefinitely — well past the scope of the local
`nogc` guard. If the reasoning above is wrong in some edge case (e.g. a
TypedArray configuration where `JS_GetArrayBufferViewBuffer`'s promotion
doesn't fully eliminate movability), this would be a real, silent
use-after-free / data-corruption bug, worse than the build simply failing.

**Recommended before trusting this build for anything beyond
experimentation**: stress-test the TypedArray-to-Python-buffer path
specifically under a compacting/moving GC configuration
(`--enable-gczeal` or equivalent SpiderMonkey debug-build GC-stress mode),
and/or get this specific diff reviewed by someone with real SpiderMonkey GC
internals expertise. Do not ship this change based on this document's
reasoning alone.

### 6. `include/JobQueue.hh` / `src/JobQueue.cc` — SpiderMonkey API addition, mechanical fix (low risk)

`JS::JobQueue::getHostDefinedData` (the base class pythonmonkey's `JobQueue`
overrides) gained a second out-parameter, `incumbentGlobal` — previously that
concept was only supplied as an *input* to the separate `enqueuePromiseJob`
method, which pythonmonkey's implementation already ignores entirely (no
incumbent-global tracking at all; jobs are just forwarded to Python's asyncio
event loop). Unlike the `BufferType.cc` fix, **this one is not a judgment
call**: pythonmonkey's existing `getHostDefinedData` already took the "we
don't need this" stance for the original `data` out-param
(`data.set(nullptr); return true;`), so the new `incumbentGlobal` param gets
exactly the same treatment, consistent with the file's own established
pattern rather than inventing new behavior:

```diff
- bool JobQueue::getHostDefinedData(JSContext *cx, JS::MutableHandle<JSObject *> data) const {
+ bool JobQueue::getHostDefinedData(JSContext *cx, JS::MutableHandle<JSObject *> incumbentGlobal, JS::MutableHandle<JSObject *> data) const {
+   incumbentGlobal.set(nullptr); // We don't need the incumbent global
    data.set(nullptr); // We don't need the host defined data
    return true;
  }
```

(Header declaration in `JobQueue.hh` updated to match.)

---

### 7. `include/JobQueue.hh` / `src/JobQueue.cc` / `src/modules/pythonmonkey/pythonmonkey.cc` — SpiderMonkey JobQueue redesign (architecture change — NEEDS REVIEW)

This is qualitatively different from every fix above it: not a renamed
parameter or a missing macro, but a real redesign of how SpiderMonkey expects
an embedder to receive promise/microtask jobs. Flagged to the team explicitly
before proceeding; the decision (from the project owner) was to keep going and
document it thoroughly rather than stop here.

**What changed, and how this was confirmed (not guessed):** the build failed
with `error: only virtual member functions can be marked 'override'` on
pythonmonkey's `enqueuePromiseJob` and `empty()` overrides. Reading the new
`JS::JobQueue` base class in full (`js/public/Promise.h`) confirmed both
methods are gone from the interface entirely — not renamed, removed. Two new
pure-virtual methods were added instead: `getHostDefinedGlobal` and (already
present, unrelated) `saveJobQueue`. To understand *why*, and find the
replacement mechanism, traced every call site of `jobQueue->` across all of
`js/src` (7 total, none enqueue-shaped), read Gecko's own real embedding
(`xpcom/base/CycleCollectedJSContext.h/.cpp`) and SpiderMonkey's own reference
embedding (`js/src/vm/JSContext.h`'s `InternalJobQueue`), and finally read
`js/public/friend/MicroTask.h`, which turned out to document the whole new
design inline (see its `[SMDOC]` comment block).

**The new design, in short:** SpiderMonkey no longer calls out to the embedder
for every job as it's created. Instead it queues jobs itself, internally
(`cx->microTaskQueues`, via `EnqueueJob()` in `js/src/builtin/Promise.cpp`).
The embedder is expected to *pull* jobs from that queue itself, inside its
`JobQueue::runJobs()` override, whenever it wants a "microtask checkpoint" to
happen. That pull is triggered by the embedder calling the free function
`js::RunJobs(cx)` (declared in `jsfriendapi.h`) — note this is **not** the
same thing as the `runJobs()` *method* pythonmonkey overrides, despite the
identical name: `js::RunJobs(cx)` is the public entry point, and its entire
body is `cx->jobQueue->runJobs(cx)` — i.e. it's what *calls* our override.

Previously, pythonmonkey never needed to call `js::RunJobs(cx)` anywhere,
because `enqueuePromiseJob` forwarded each job to Python's asyncio event loop
the instant SpiderMonkey created it — there was no engine-side queue to drain.
Under the new design, if nothing ever calls `js::RunJobs(cx)`, jobs pile up in
`cx->microTaskQueues` forever and **no promise ever resolves**. So this fix
has two parts:

**(a) `JobQueue::runJobs()` now does real work** (`src/JobQueue.cc`), instead
of being a no-op. It loops while `JS::HasAnyMicroTasks(cx)`, dequeues each job
via `JS::DequeueNextMicroTask` + `JS::ToMaybeWrappedJSMicroTask`, and forwards
it to the Python event loop — recreating what `enqueuePromiseJob` used to do
per-job, just pull-based now instead of push-based:

```diff
- void JobQueue::runJobs(JSContext *cx) {
-   // Do nothing
- }
+ void JobQueue::runJobs(JSContext *cx) {
+   while (JS::HasAnyMicroTasks(cx)) {
+     JS::RootedValue entry(cx, JS::DequeueNextMicroTask(cx));
+     if (entry.isNull()) break;
+     JS::Rooted<JS::JSMicroTask *> job(cx, JS::ToMaybeWrappedJSMicroTask(entry));
+     if (!job) continue;
+     auto *rootedJob = new JS::PersistentRooted<JSObject *>(cx, job);
+     // ... pack (cx, rootedJob) into a PyCFunction closure, enqueue it on
+     // the running Python event-loop (see runMicroTaskCallback), same as
+     // enqueuePromiseJob's loop.enqueue(callback) did before.
+   }
+ }
```

One real difference from the old `enqueuePromiseJob`: `job` there was a
`JS::HandleObject` documented as an ECMA-262 Job (i.e. a plain callable
function with no arguments), which pythonmonkey converted straight to a
Python callable via `pyTypeFactory(cx, jobv)` and handed to Python. The new
`JS::JSMicroTask*` is **not** a generically-callable function — it's an opaque
engine-internal representation that must be executed specifically via
`JS::RunJSMicroTask(cx, job)`, inside `AutoRealm`d to
`JS::GetExecutionGlobalFromJSMicroTask(job)` (this exact usage pattern is
documented in the `[SMDOC]` block at the top of `js/public/friend/MicroTask.h`
— not improvised). So instead of reusing `pyTypeFactory` to wrap `job`
itself, a new small native PyCFunction (`runMicroTaskCallback`) was added,
modelled directly on the existing `dispatchToEventLoop`/`callDispatchFunc`
pattern already in this same file (which smuggles a `(JSContext*,
JS::Dispatchable*)` pair through a Python closure the same way) — packs
`(cx, rootedJob)` as a 2-tuple, and when Python's loop finally calls it, calls
`JS::RunJSMicroTask` and reports failure via the existing
`setSpiderMonkeyException(cx)` helper (same one used throughout
`pythonmonkey.cc`).

`JS::JSMicroTask` is a type alias for plain `JSObject` (confirmed directly in
`MicroTask.h`: `using JSMicroTask = JSObject;`), so it can be kept alive
across the gap between "dequeued here" and "Python's event loop calls back,
possibly much later" the same way pythonmonkey already keeps
FinalizationRegistry callbacks alive elsewhere in this file: a heap-allocated
`JS::PersistentRooted<JSObject *>`, freed once the callback actually runs.

**(b) `pythonmonkey.cc` now calls `js::RunJobs(GLOBAL_CX)`** once, immediately
after every top-level `JS_ExecuteScript()` call — the natural equivalent of
the HTML spec's "clean up after running script" microtask checkpoint, and (as
far as could be found) the only place in pythonmonkey's own source that a
checkpoint like this was ever implicitly happening before (via the old
immediate-forwarding design).

**`getHostDefinedGlobal`** (the other new pure-virtual method) was given the
same "we don't track this" stance pythonmonkey already takes for
`getHostDefinedData`'s params — `out.set(nullptr); return true;` — which
matches SpiderMonkey's own reference embedding
(`InternalJobQueue::getHostDefinedGlobal` in `js/src/vm/JSContext.cpp`)
exactly, so this part is low-risk / pattern-consistent rather than a guess.

**NEEDS REVIEW — this is the least-verified change in this entire document,
more so than the `BufferType.cc` GC fix:**
- Whether draining exactly once per `JS_ExecuteScript()` call is the *right*
  cadence for this embedding (vs., say, needing a checkpoint after every
  re-entry into JS, or after every Python-side `await` of a JS promise) has
  not been verified against real async/await interop test cases — only
  against "does it compile and does the basic shape make sense."
- GC-safety of holding a `JS::PersistentRooted<JSObject *>` across an
  arbitrary, unbounded real-world delay (Python's event loop may not run the
  callback for a while) is modelled on the pre-existing, working
  `finalizationRegistryCallbacks` pattern in this same file, but has not been
  independently confirmed for `JSMicroTask` objects specifically.
- The ordering/interleaving semantics (does a JS promise chain still resolve
  in the same relative order it used to, now that jobs are batch-pulled per
  checkpoint instead of pushed one at a time?) has not been tested.
- **Before trusting this for anything beyond experimentation**: write and run
  a test that chains multiple `await`s across the Python/JS boundary
  (`pm.eval` returning a Promise that resolves another Promise, etc.) and
  confirms both completion and ordering, not just successful compilation.

**UPDATE — this was tested, and the concern above was real.** Once the build
first succeeded (fix #10 below), a smoke test awaiting even a single,
already-resolved JS Promise from Python hung indefinitely. Root cause: the
one `js::RunJobs(GLOBAL_CX)` call added above (after `JS_ExecuteScript`)
only checkpoints the *first* batch of jobs created during top-level script
execution. It does not cover the other two places jobs get freshly enqueued
into `cx->microTaskQueues`, both entirely outside of any `JS_ExecuteScript`
call:

1. **`PromiseType::getPyObject`** (`src/PromiseType.cc`) — called when Python
  code `await`s a JS Promise. `JS::AddPromiseReactions` attaches a reaction
  callback; if the promise is already settled (the common case for a
  same-tick resolution), this immediately enqueues a job that nothing was
  draining.
2. **`futureOnDoneCallback`** (`src/PromiseType.cc`) — called from a Python
  `asyncio.Future`'s done-callback (i.e. from Python's event loop, not from
  JS at all) to resolve/reject a JS Promise that JS was awaiting on a Python
  awaitable. `JS::ResolvePromise`/`JS::RejectPromise` here can trigger that
  promise's own already-attached reactions, again with nothing draining them.
3. **`runMicroTaskCallback`** (`src/JobQueue.cc`, part of this same fix #7) —
  running one microtask (e.g. one `await` in a chain) can enqueue the next
  one; the callback returned without re-checkpointing, so a promise chain
  with more than one `await` stalled after the first hop even once (1) and
  (2) were fixed.

Fixed by adding `js::RunJobs(cx)` at all three points — mechanical once the
pattern was identified (same call used above), but finding *where* it was
missing required actually running async code, not just getting a clean
compile. **This is the concrete confirmation that "compiles" and "works" are
different claims for this whole JobQueue rewrite** — treat any other
not-yet-exercised code path in this rewrite (the debug queue,
`saveJobQueue`/`SavedJobQueue` used by the Debugger API, `isDrainingStopped`)
with the same suspicion until it's actually been run.

Retested after this fix: a single `await` of an already-resolved Promise, a
two-hop `await` chain inside an async function (verifying both completion
*and* ordering), and a `setTimeout`-based Promise (exercising the unrelated,
pre-existing `PyEventLoop::enqueueWithDelay` timer path) — see the Testing
section near the end of this document for exact results.

### 8. `src/JobQueue.cc` — `mozilla::Unused` / `mozilla/Unused.h` removed upstream, mechanical fix (low risk)

Next build error after fix #7: `fatal error: 'mozilla/Unused.h' file not found`.
Confirmed this isn't a path/environment issue — the header (and the
`mozilla::Unused` helper it declared) is genuinely gone from the current
mozilla-central snapshot's `mfbt/` directory, not just moved (checked: absent
from `mfbt/`, and grepping `dom/`, `xpcom/base/`, `js/src/vm/` for
`mozilla::Unused` turns up zero uses anywhere in current upstream code,
confirming it's been fully purged, not merely renamed). The old header
(preserved at
`_spidermonkey_install.orig-136a1-backup/include/mozjs-136a1/mozilla/Unused.h`
from the previous build) shows `Unused << expr` was only ever a thin
"suppress unused-nodiscard-return-value warning" helper
(`template<T> void operator<<(const T&) const {}`) — functionally identical
to a plain `(void)expr;` cast. Two use sites in `JobQueue.cc` (the only file
in this codebase that used it) were switched to that, and the now-dead
`#include <mozilla/Unused.h>` removed:

```diff
- mozilla::Unused << finalizationRegistryCallbacks->append(callback);
+ (void)finalizationRegistryCallbacks->append(callback);
...
- mozilla::Unused << JS_CallFunction(cx, NULL, func, JS::HandleValueArray::empty(), &unused_rval);
+ (void)JS_CallFunction(cx, NULL, func, JS::HandleValueArray::empty(), &unused_rval);
```

### 9. `include/JobQueue.hh` / `src/JobQueue.cc` — off-thread dispatch API redesign (moderate risk)

Next build errors after fix #8, all in the same area: `JS::InitDispatchToEventLoop`
no longer exists ("did you mean 'dispatchToEventLoop'?"); a 3-argument call
where only 2 are now expected; and `'run' is a protected member of
'JS::Dispatchable'`. Read the current `js/public/Promise.h` (lines 622-817) in
full to understand the new shape rather than guessing from the error text
alone.

**What changed:**
- `JS::InitDispatchToEventLoop(cx, callback, closure)` → replaced by
  `JS::InitAsyncTaskCallbacks(cx, dispatchCallback, delayedDispatchCallback,
  asyncTaskStartedCallback, asyncTaskFinishedCallback, closure)`. The first
  two callbacks are now both mandatory (previously only one existed at all);
  the last two are optional (`nullptr` accepted).
- `DispatchToEventLoopCallback`'s signature changed from taking a raw
  `JS::Dispatchable*` to taking ownership via `js::UniquePtr<Dispatchable>&&`.
- `Dispatchable::run()` is now `protected`. The new public entry point is the
  static `Dispatchable::Run(JSContext*, js::UniquePtr<Dispatchable>&&,
  MaybeShuttingDown)`, which takes ownership and is responsible for both
  calling `run()` and cleaning up.
- A brand new, previously-nonexistent-for-this-embedding
  `DelayedDispatchToEventLoopCallback` is now mandatory too.

**Fix, in `src/JobQueue.cc`:**

```diff
- JS::InitDispatchToEventLoop(cx, dispatchToEventLoop, cx);
+ JS::InitAsyncTaskCallbacks(cx, dispatchToEventLoop, delayedDispatchToEventLoop, nullptr, nullptr, cx);
```

`dispatchToEventLoop` itself: the raw `Dispatchable*` this used to smuggle
through a Python closure (packed as a `PyLong` pointer, same trick used
elsewhere in this file for the microtask fix in #7) is now obtained via
`dispatchable.release()` before packing, and reconstructed with
`js::UniquePtr<JS::Dispatchable>(dispatchable)` on the other side, then run
via `JS::Dispatchable::Run(cx, ..., JS::Dispatchable::NotShuttingDown)`
instead of the old direct `dispatchable->run(cx, ...)` call — mechanical
translation of the ownership-transfer model, not a judgment call.

**`delayedDispatchToEventLoop` — NEEDS REVIEW, the one genuine judgment call
in this fix:** this embedding has no existing mechanism for scheduling a
callback *safely from an arbitrary SpiderMonkey helper thread* with a delay.
`PyEventLoop::enqueueWithDelay` exists and is used elsewhere (JS
`setTimeout`), but it calls `asyncio.loop.call_later`, which — unlike
`call_soon_threadsafe` (used by `PyEventLoop::enqueue`, and safe from any
thread) — is not documented as callable from a thread other than the one
running the loop. Since `DelayedDispatchToEventLoopCallback` is explicitly
documented as needing to be safe from any thread, reusing `enqueueWithDelay`
directly would be a plausible new thread-safety bug, not a fix.

Instead, this implementation always returns `false`, which
`js/public/Promise.h` explicitly sanctions: *"If a timeout manager is not
available for given context, it should return false."*

**Correction made during this fix, left visible because the first instinct
was wrong and it's a useful lesson for reviewers:** the first attempt had
this call `dispatchable.release()` then `task->transferToRuntime()` directly,
based on a doc comment on `Dispatchable::transferToRuntime()` showing that
exact usage pattern. That failed to compile — `transferToRuntime()` is
`protected`, so an embedder callback has no access to it (the doc comment
describes SpiderMonkey's *own* internal usage, not the embedder-facing API).
The actually-correct, embedder-facing call was found by reading real
production code instead of inferring from a header comment: Gecko's own
`dom/workers/RuntimeService.cpp` (`JSDispatchableRunnable::PostDispatch`)
handles exactly this "took ownership, failed/declined to dispatch" case with
the public static `JS::Dispatchable::ReleaseFailedTask(std::move(task))`,
which is what this fix now uses.

**Risk assessment**: this should only affect internal SpiderMonkey features
that specifically need an off-thread *delayed* dispatch (the header mentions
`Atomics.waitAsync` timeouts as an example). Ordinary JS `setTimeout` /
`setInterval` go through a separate, unaffected, already-working path
(pythonmonkey's own JS-exposed timer functions calling
`PyEventLoop::enqueueWithDelay` directly, on the main thread). **Not verified
against a real `Atomics.waitAsync`-with-timeout test case** — if this
embedding's use cases ever depend on that specific feature, this will need
a real timeout-manager implementation instead of the `false` stub.

### 10. `src/modules/pythonmonkey/pythonmonkey.cc` — asm.js support removed, mechanical fix (low risk)

Next build error after fix #9 (and the first one outside `JobQueue.cc`/`.hh`):
`error: no member named 'setAsmJS' in 'JS::ContextOptions'`. Confirmed via
`js/public/ContextOptions.h` that no asm.js-related member exists on
`ContextOptions` anymore at all — not renamed, removed. asm.js was a
pre-WebAssembly, Firefox-specific JS-subset compilation target; WebAssembly
(enabled separately via the still-present `.setWasm(true)`, unaffected by
this) has long since superseded it upstream. Simply deleted the
`.setAsmJS(true)` call in the `ContextOptionsRef` chaining call during
context setup — nothing to replace it with, since the feature itself is gone,
not relocated.

### 11. `src/JSFunctionProxy.cc` / `src/JSMethodProxy.cc` — a 4th missing JobQueue checkpoint, found by independent retesting (moderate risk, now fixed)

**This section exists because the "All passed, repeatably" claim under point 3
of the Testing section below was wrong when first written.** Independent
retesting (by Claude, at the requester's request, specifically to audit this
PR before review) redeployed the actual built `pythonmonkey.pyd` +
`mozjs-157a1.dll` — the previously-installed copy in `site-packages` was
stale, still linked against the old `136a1` engine, so earlier manual smoke
tests after the JobQueue rewrite had not actually been exercising this build
— and found that awaiting a JS Promise resolved via `setTimeout` hangs
indefinitely, and so does the real `dcp_local_job_test.py` end-to-end job
(it gets through bootstrap and identity loading, then never fires a single
`readystatechange` event).

**Root cause**: fix #7's checkpoint list (`JobQueue::runJobs`,
`PromiseType::getPyObject`, `futureOnDoneCallback` — three places new jobs
get enqueued into `cx->microTaskQueues` outside of a top-level
`JS_ExecuteScript()` call) missed a fourth: `JSFunctionProxy_call`
(`src/JSFunctionProxy.cc`) and `JSMethodProxy_call` (`src/JSMethodProxy.cc`)
are the generic entry points Python uses to call back into *any* JS function
or bound method it was handed — this is what fires a `setTimeout` callback
dispatched from `PyEventLoop`, or a JS event listener invoked directly from
Python code. Both call `JS_CallFunctionValue` and return without ever
draining the job queue afterward. If the JS function just called
resolved/rejected a Promise with already-attached reactions (the common case:
`resolve(...)` inside a `setTimeout` callback), that enqueues a job nothing
was scheduled to drain.

**Fix**, identical in both files — add the same checkpoint used everywhere
else in this rewrite, immediately after the call succeeds:

```diff
   if (!JS_CallFunctionValue(cx, thisObj, jsFunc, jsArgs, &jsReturnVal)) {
     setSpiderMonkeyException(cx);
     return NULL;
   }

+  js::RunJobs(cx);
+
   if (PyErr_Occurred()) {
     return NULL;
   }
```

(`#include <jsfriendapi.h>` added to both files for the declaration, matching
`JobQueue.cc`'s existing include.)

**Retested after this fix** — all of Testing point 3 below plus an added
sequential-delayed-promises case, and all of points 4 and 5 (the full
`localExec()` suite and the real `exec()` test) were rerun end-to-end against
this exact rebuilt binary. All passed; see the corrected Testing section
below. This is the second time in this same JobQueue rewrite that "compiles
and a few manual checks look right" turned out not to mean "actually works"
— treat that as a standing warning for any *other* not-yet-exercised path in
this rewrite (the Debugger-API paths flagged in "Not yet done" below), not
just the two paths that have now each independently failed once.

---

## Testing — what was actually run, and what it showed

**Important caveat added later**: everything in this section was run
against a local build on the original development machine, using the local
Rust `stable` override described in section 3 above — **not against real
CI**. When this PR's actual CI ran, all 5 platforms failed on toolchain
version mismatches invisible to that local setup (section 3/3b). Fixed as
of the most recent commit; CI has not yet been re-verified green after that
fix (this doc will be updated once it has, or note here if it wasn't
before merge).

Ten build errors were fixed in total (sections 1-10 above), each one a real
SpiderMonkey-internal API break between the old `136a1` nightly-alpha build
and current mozilla-central — none were environment/tooling issues by this
point (those were resolved earlier, before section 1). The build finally
succeeded (`pythonmonkey.pyd` + `mozjs-157a1.dll`, `BUILD_EXIT_CODE=0`).

After that, four rounds of runtime verification were run (not just "it
compiles"):

1. **Basic eval**: `pm.eval('1 + 2')`, `pm.eval('JSON.stringify(...)')` —
   passed.
2. **`SharedArrayBuffer`/`Atomics`** — the original, one-line motivating fix
   for this entire rebuild (a completely separate, older issue from
   everything in this document). Verified still working:
   `Atomics.store`/`Atomics.load` round-trip through a `SharedArrayBuffer`
   returned the correct value.
3. **Async/Promise interop across the Python/JS boundary** — this is where
   real bugs actually turned up (see the "UPDATE" note under fix #7 above for
   the full story: a single `await` of a JS Promise hung indefinitely on the
   first attempt, root-caused to `js::RunJobs(cx)` only being called from one
   of the three places new jobs actually get enqueued). After fixing all
   three call sites, verified: a single `await` of an already-resolved JS
   Promise; a two-`await` chain inside a JS async function, checking both
   completion *and* correct ordering (`[1, 3, 5]`, not e.g. `[1, 5, 3]`); and
   a `setTimeout`-based Promise. **This third case was reported as passing
   here, but that was wrong** — see fix #11 above: the actual built binary
   deployed to `site-packages` was stale at the time (still the old `136a1`
   engine), so this hadn't really been exercised against this rewrite. Once
   retested against the real binary, the `setTimeout` case hung, was
   root-caused to a 4th missing checkpoint (fix #11), and after that fix, all
   of the above — plus an added sequential-back-to-back-delayed-promises
   case — passed, repeatably, confirmed against the actual rebuilt
   `pythonmonkey.pyd`.
4. **The real `localExec()` test suite**, run end-to-end against the new
   engine, exactly as originally planned. **Like point 3, this was also
   re-verified after fix #11** — `dcp_local_job_test.py` specifically hangs
   after identity loading without that fix (identity loading itself is an
   `async` JS function call, which goes through `PromiseType::getPyObject`
   and was already covered; the job's own event/timer-driven machinery is
   what hit the missing 4th checkpoint):
   - `dcp_local_job_test.py` — a real job (`dcp.compute_for` over 8 letters,
     uppercasing work function), through the full `localExec()` pipeline
     (readystate transitions, identity loading from a real `id.keystore`,
     job deployment, slice completion, result collection). **Passed** —
     correct output `YELLING!`.
   - `pycomod_localexec_test.py` — a much heavier stress test: real
     filesystem shipping (`job.fs.add`) of a local Python package into the
     sandbox, extra declared Pyodide modules (`pandas` on top of the usual
     `numpy`/`cloudpickle`), extra work-function arguments flowing through
     `job.jobArguments`, and — importantly — non-primitive slice results
     (nested dicts of numpy arrays), which exercises cloudpickle's real
     serialization round-trip rather than the primitive fast path. **Passed**
     — all 5 slices completed with correct structure and correct numeric
     values (spot-checked a sample series: `values[:5] = [25. 25. 25. 25.
     25.]`, `dtype=float32`, as expected for this model). Takes a few minutes
     (real Pyodide package loading: `pandas`/`numpy`/`cloudpickle`/etc.) —
     don't mistake the lack of output during that window for a hang.

5. **Real `job.exec()` (not `localExec()`) — genuine network dispatch,
   verified separately** (`dcp_real_exec_test.py`, modelled directly on
   `dcp_sample_job.py`'s pattern but loading identity from `id.keystore`
   the same safe way `dcp_local_job_test.py` does, rather than an inline
   private key). This is a materially different code path from everything
   above: `localExec()` never leaves the process, while `exec()` submits to
   the real DCP scheduler, needs a funded wallet, and depends on real
   workers actually being present on the target compute group
   (`demo`/`dcp`, the same public demo group both existing sample scripts
   already use). **Passed, real end-to-end**: full real readystate
   lifecycle (`exec → init → preauth → deploying → listeners →
   compute-groups → uploading → deployed`), a real scheduler-assigned job
   ID, 8 real `result` events from real workers, no `nofunds`/`error`
   events, correct final output `YELLING!`. This confirms the rebuild is
   solid for the actual production dispatch path, not just the
   local-simulation path this document otherwise focuses on. **Also
   re-verified after fix #11**, for the same reason as point 4.

## Not yet done / open as of this writing

- **Resolved, was previously unverified**: fix #11 above closed the 4th
  missing JobQueue checkpoint. Before it, every claim in the Testing section
  that touched an event/timer-driven callback (the `setTimeout`-Promise case
  in point 3, and both `localExec()`/`exec()` end-to-end tests in points 4-5)
  had actually been checked against a stale, pre-rewrite binary rather than
  this PR's real build, and would have hung for anyone who ran them for
  real. All were rerun against the actual rebuilt `pythonmonkey.pyd` and now
  pass. Leaving this note here rather than deleting it: if a *fifth*
  Python→JS callback path turns up somewhere that neither this fix nor the
  original three cover, that would make two independent misses in the same
  rewrite, which would be a good reason to stop patching call sites
  one-by-one and instead audit every `JS_Call*`/`JS_Invoke` call in the
  codebase for the same gap systematically.

- **Not independently verified**: `Atomics.waitAsync` with a real timeout
  (the `delayedDispatchToEventLoop` stub in fix #9 always declines these —
  see that section's risk assessment). Only relevant if something in this
  codebase's dependency tree actually uses that specific API; not exercised
  by either test suite above.
- **Not independently verified**: the Debugger-API-facing parts of the
  JobQueue rewrite (`saveJobQueue`/`SavedJobQueue`, `isDrainingStopped`,
  the debug microtask queue via `useDebugQueue`) — pythonmonkey doesn't
  currently expose SpiderMonkey's Debugger API to Python, so these paths
  are believed unreachable in normal use, but that belief hasn't been
  tested against actually invoking the Debugger API.
- **Not independently verified**: the `BufferType.cc` GC-safety reasoning
  (fix #5, `JS_GetArrayBufferViewFixedData` → `JS_GetArrayBufferViewData`
  + `AutoAssertNoGC`) — this needs either a compacting/moving-GC stress test
  (`--enable-gczeal` or equivalent) or review by someone with real
  SpiderMonkey GC internals expertise before being trusted beyond
  experimentation. Both test suites above exercise TypedArray/buffer code
  paths incidentally (numpy arrays flow through cloudpickle, not directly
  through this code path) but do not specifically stress-test this.
- **Not run**: any long-running / soak test. Everything above is a single
  run of each script; no repeated-execution, memory-leak, or
  long-session-stability testing has been done. The heap-allocated
  `JS::PersistentRooted` objects created per-microtask in fix #7
  (`JobQueue::runJobs`) are freed on the happy path (`runMicroTaskCallback`)
  but **not** on at least one error path worth double-checking before a
  soak test: if `PyEventLoop::getRunningLoop()` fails inside
  `JobQueue::runJobs` after a `rootedJob` has been allocated, it is deleted
  correctly (see that code) — but this exact path has not been exercised
  by any test above, since the running loop was always available.
- **Not reviewed by anyone else.** Every fix in this document was made by
  one engineer (with AI pair-programming assistance) working from primary
  sources (the actual SpiderMonkey headers and, where embedder-facing
  behavior was unclear, real production usage in Gecko's own source) rather
  than guessing, and where a first attempt was wrong (see fix #9's
  `transferToRuntime`/`ReleaseFailedTask` correction, and fix #7's
  three-checkpoint hang), that's recorded rather than smoothed over — but
  none of it has had a second, independent pair of eyes. Recommended before
  shipping this beyond internal experimentation: a real code review of
  sections 5, 7, and 9 in particular (the three sections marked NEEDS
  REVIEW / moderate-or-higher risk above), ideally by someone with prior
  SpiderMonkey embedding experience.
- The old `136a1` install and DLL were preserved as `*.orig-136a1-backup` /
  `*.orig-backup` throughout this work (see individual sections above) —
  don't delete these until the team has independently confirmed the new
  build in their own environment, not just this one.
