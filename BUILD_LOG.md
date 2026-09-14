# pythonmonkey local rebuild — build log

## FINAL: `localExec()` under Bifrost2/pythonmonkey — fully working, confirmed end-to-end

```
$ python dcp_local_job_test.py
...
YELLING!
```

Exit code 0. Real `dcp.compute_for()` + `job.localExec()`, a real
Python/Pyodide work function, real network communication with the real
DCP scheduler and package manager, running under pythonmonkey -- the
original goal of this entire investigation. Three more bugs (all in
`localExec()`'s own job-completion detection, on top of the WebSocket fix
below) were found and fixed to get from "real results delivered" to
"process actually exits with the right value" -- full details in
`localexec_patch/STATUS.md`'s "DONE" section at the top.

## Real Pyodide jobs run end-to-end under pythonmonkey (WebSocket fix)

After this file's original C++ rebuild (shared memory) and a series of
Bifrost2/dcp-client fixes documented below, one issue remained: a real,
reproducible bug in Distributive's `packages.distributed.computer`
package-manager service that only pythonmonkey ever hit, because
dcp-client hard-codes pythonmonkey to long-poll forever (every other
platform upgrades to WebSocket almost immediately, sidestepping it).
**Fix: gave pythonmonkey a real `WebSocket` implementation** — two new
builtin modules (`WebSocket.js` + `WebSocket-internal.py`, the latter
backed by `aiohttp`), following the exact existing pattern of
`XMLHttpRequest.js`/`XMLHttpRequest-internal.py`. Loaded dynamically via
`require()` at import time — **no C++ rebuild needed** for this one, only
for the original `SharedArrayBuffer` fix below. Full details, the two
bugs found while building the polyfill, and the resulting **first-ever
correct end-to-end Pyodide execution** (`dcp_local_job_test.py`'s 8
slices all returned the right letters) are in
`localexec_patch/STATUS.md`'s "BREAKTHROUGH" section at the top.

## RESULT: BUILD SUCCEEDED, FIX CONFIRMED WORKING

`pythonmonkey.pyd` and `mozjs-136a1.dll` built successfully and copied into
the installed `dcp` package's `site-packages/pythonmonkey/` (originals
preserved as `*.orig-backup` in that same directory). Confirmed directly:

```
typeof SharedArrayBuffer: function   (was "undefined")
typeof Atomics: object               (was "undefined")
WebAssembly.Memory shared test: OK   (was "FAIL: shared memory is disabled")
```

The one-line fix (`creationOptions.setSharedMemoryAndAtomicsEnabled(true);`
in `src/modules/pythonmonkey/pythonmonkey.cc`) works exactly as expected.

**Ran the original Python/Pyodide work-function test
(`dcp_local_job_test.py`) — the actual blocker is confirmed resolved.**
`LinkError: shared memory is disabled`, `API._pyodide is undefined`, and
the resulting infinite "Main module was provided before job assignment"
retry loop are **all gone**. Pyodide's WASM module now links and
initializes successfully — Python work functions can actually start
running under `localExec()` now, which was impossible before this fix no
matter what else got patched at the dcp-client/JS level.

The run then hit a **different, unrelated issue**: `ENOSLICEHANDLER: Must
specify the slice handler using dcp.set_slice_handler(fn)`, repeated
per-slice, followed by `Exception: Wait called before exec()`. This turned
out to be two real Bifrost2/dcp-client bugs (not pythonmonkey/build
issues, not usage errors) — see `localexec_patch/STATUS.md` for the full
writeup:

1. `job.py`'s `Job` class only builds the real Bifrost2-wrapped work
   function script (the thing that actually calls
   `dcp.set_slice_handler()`) inside `_before_exec()`, which is wired up
   for `exec()` but never for `localExec()`. Fixed by adding an explicit
   `localExec()` method to `job.py`.
2. The earlier session's chained-require fix dropped BravoJS's own
   require-object methods (`.id` etc.), causing `require.id is not a
   function` inside BravoJS's own module system, which aborted job
   assignment and permanently polluted the shared JS global's
   `module.main` — producing an infinite `describe`/`assign`/reject retry
   loop that looked unrelated. Fixed by copying BravoJS's require
   properties onto the merged require.

**FINAL STATUS: all Bifrost2/pythonmonkey bugs found and fixed.**
Decisive proof: the exact `job.js_ref.workFunctionURI`/`jobArguments`
payload that a fixed Bifrost2 `_before_exec()` generates was dumped to
JSON and replayed onto a fresh job in Node.js
(`debug-dcp-worker/node_pyodide_replay_test.js`), which called
`localExec()` and **completed fully: `Job completed: YELLING!`** — proving
the generated Python/Pyodide work-function harness and argument-vector
construction are entirely correct, with no remaining Bifrost2-level bugs.

The earlier XHR-restoration hypothesis in this section was WRONG (traced
in `localexec_patch/STATUS.md`: the real path is BravoJS's own
dependency-fetch protocol → `ModuleCache.fetchModule` →
`dcp4.packageManager.request("fetchModuleURL")`, a socket.io RPC call —
not `pyodide-core.js`'s fetch relay at all). The actual remaining failure
under pythonmonkey is a real HTTP `404` then `502` from Distributive's
`packages.distributed.computer` package-manager service's own backend,
reproduced even with plain `aiohttp` and plain Node `https` with no
pythonmonkey or dcp-client involved at all — while the identical request
pattern against `scheduler.distributed.computer` (the main scheduler)
sustains dozens of round-trips without ever failing. This is external
infrastructure, not a pythonmonkey engine bug and not a shared-memory
issue — **`SharedArrayBuffer`/`Atomics` remain fixed and confirmed working
in every run checked, old and new, with zero `LinkError`s anywhere.** See
`localexec_patch/STATUS.md`'s "Pyodide next issues" section for the full
evidence trail and recommendation (retry after a cooldown; check
`packages.distributed.computer`'s health directly with whoever operates
it, since this session's own heavy automated testing may have contributed
to the degraded state observed). A separately-checked background task run
(`~/DCP/keepalive_test.out`, task `bf4xptw1p`) predates the
localExec()/require.id fixes and shows the old, now-fixed
`ENOSLICEHANDLER` failure — superseded, kept only as historical evidence
that shared memory itself was never the problem in that run either.


Goal: rebuild pythonmonkey locally with one added line
(`creationOptions.setSharedMemoryAndAtomicsEnabled(true);` in
`src/modules/pythonmonkey/pythonmonkey.cc`) to unblock Pyodide/Python work
functions under `localExec()`. See `localexec_patch/STATUS.md` in this repo
for the full context of why this fix is needed.

Repo cloned to: `C:\Users\danie\DCP\pythonmonkey-src`

## Exact build requirements (from reading `setup.sh` in full)

- **Rust pinned to exactly 1.85** (`--default-toolchain 1.85`) — not
  whatever `rustup` installs by default (that gave 1.98.1).
- **cbindgen** via `cargo install cbindgen`.
- **Poetry 1.7.1**, plus the `poetry-dynamic-versioning` plugin.
- **clang/LLVM** — the build targets `$(clang --print-target-triple)`,
  i.e. SpiderMonkey's Windows build uses clang (clang-cl ABI), not plain
  MSVC `cl.exe` directly.
- **On Windows, the script installs NONE of its own dependencies** — it
  explicitly skips that step (`"Dependencies are not going to be installed
  automatically on Windows."`) and expects everything already present in
  an MSYS2/MozillaBuild-style bash environment: `cmake`, `m4`, `unzip`,
  `wget`, `curl`, plus Python for Mozilla's own `mach`/`mozbuild` build
  system.
- Downloads the **entire Firefox source tree** as a zip from
  `mozilla-firefox/firefox` at the commit in `mozcentral.version`, applies
  ~10 `sed` patches to it (SpiderMonkey/PythonMonkey-specific fixes), then
  builds via `configure && make -j$CPUS` inside `js/src`.
- Known risk not addressed by the script: **Python version**. Mozilla's
  `mach`/`mozbuild` build tooling has historically required an older
  Python (3.8–3.11 range). This machine has Python 3.14.7. Not yet
  confirmed whether mozilla-central's current build system tolerates
  3.14 — this is a real, unquantified risk until actually attempted.

## Progress

- [x] Rust installed via `rustup-init.exe` (already present: rustc 1.98.1,
      installed before this session started).
- [x] NASM installed via `winget install NASM.NASM` (3.02).
- [x] MSYS2 installed via `winget install MSYS2.MSYS2`, at `C:\msys64`.
- [x] Rust 1.85 toolchain pinned (`rustup toolchain install 1.85`) —
      confirmed via `rustup toolchain list`: both `stable` (1.98.1,
      default) and `1.85-x86_64-pc-windows-msvc` now present. Will need
      `rustup override set 1.85` (or `+1.85` per-command) inside the
      pythonmonkey repo checkout so its build actually uses 1.85, not the
      1.98.1 default.
- [x] cbindgen installed via `cargo install cbindgen`.
- [x] "C++ Clang Compiler for Windows" VS component
      (`Microsoft.VisualStudio.Component.VC.Llvm.Clang`) added to the
      existing VS Build Tools 2026 install via
      `vs_installer.exe modify --add ...` — this is what CMake's
      `-T ClangCL` toolset (used by `build.py` on Windows) actually needs;
      a standalone `winget install LLVM.LLVM` alone would NOT have
      provided this VS-integrated toolset.
- [~] LLVM.LLVM (standalone command-line clang) — install kicked off via
      winget, still running as of this log entry. Not certain yet whether
      this is even needed in addition to the VS ClangCL component above
      (setup.sh's own `clang --print-target-triple` call wants a `clang`
      on PATH — the VS component may or may not add a plain `clang.exe` to
      PATH by itself, so keeping this standalone install as a safety net).
- [~] MSYS2 packages (`m4 unzip wget curl base-devel`) — install kicked
      off via `pacman -S`, still running as of this log entry.
- [ ] Poetry 1.7.1 + poetry-dynamic-versioning — **currently believed
      unnecessary**. Found that `build.py` (the actual build driver) is a
      plain, directly-runnable Python script (`python build.py`) — Poetry
      is only the conventional wrapper (`poetry build` invokes this as a
      custom build-backend script), not a hard requirement. `build.py`
      itself calls `bash ./setup.sh` (if `_spidermonkey_install/lib`
      doesn't already exist) then does the CMake build then copies
      `pythonmonkey.pyd`/`mozjs-*.dll` into `python/pythonmonkey/`. Plan:
      skip Poetry entirely, run `python build.py` directly, then manually
      copy the two output files into the already-installed `dcp` package's
      `site-packages/pythonmonkey/` — no full `pip install`/wheel-build
      round-trip needed for what we're trying to confirm.
- [x] `mozcentral.version` checked: pinned Firefox commit is
      `6bca861985ba51920c1cacc21986af01c51bd690`. Not yet checked exact
      download size, but full mozilla-firefox source archives are
      routinely several hundred MB compressed / multiple GB uncompressed —
      expect this step alone to take a while depending on network speed.
- [x] **`setup.sh` fully succeeded — SpiderMonkey itself is built and
      installed** to `_spidermonkey_install/lib` (confirmed: `build.py`'s
      `ensure_spidermonkey()`, which checks for exactly that directory
      before deciding whether to (re)run `setup.sh`, is no longer being
      re-entered — `build.py` now proceeds straight to `run_cmake_build()`
      on every rerun). This took 9 rounds of Windows-environment fixes
      (see "Notes as we go" below for the full trail): OSTYPE detection,
      `python3` shim (twice, for two different tools), idempotent
      extraction, ATL/MFC components (three sub-issues), Python 3.11 for
      Mozilla's own build tooling, a real bug in mozbuild's `shellutil.py`
      plus a self-inflicted `MOZILLABUILD` env var issue, and finally a
      MinGW-w64 `make` requirement. None of the fixes needed were related
      to the actual one-line pythonmonkey change — all Windows/environment
      friction from running Mozilla's Linux/macOS-first build tooling
      without the official "MozillaBuild" package.
- [~] `run_cmake_build()` (the second, separate build stage — compiling
      pythonmonkey's own C++ extension via CMake's `-T ClangCL` toolset,
      distinct from SpiderMonkey's own `make`-based build above) — in
      progress. First attempt failed with `MSB8020: The build tools for
      ClangCL ... cannot be found` — confirmed the earlier, non-elevated
      "C++ Clang Compiler for Windows" VS component install attempt
      (way back near the start of this log) never actually took effect,
      for the same silent-elevation reason later diagnosed for ATL/MFC.
      Fixed the same way: `Start-Process -Verb RunAs` — confirmed this
      time via `VC\Tools\Llvm\x64` actually existing on disk afterward.
- [ ] One-line fix applied to `pythonmonkey.cc` — not yet applied. Plan:
      apply it **before** the first full build (not after), since
      `ensure_spidermonkey()` only skips the *SpiderMonkey* build on
      re-runs, and the CMake/`pythonmonkey.cc` compile step is fast
      regardless — no benefit to building once without the fix first.
- [ ] Built `.pyd`/`.dll` copied into the installed `dcp` package's
      pythonmonkey (`C:\Users\danie\AppData\Roaming\Python\Python314\site-packages\pythonmonkey\`,
      replacing the existing `pythonmonkey.pyd` and `mozjs-136a1.dll`) and
      confirmed `SharedArrayBuffer`/`Atomics` become defined (rerun the
      isolated `sab_check.py`-style probe from `localexec_patch/STATUS.md`'s
      "Pyodide / shared memory" section before attempting the full job
      test, to fail fast if the engine build itself didn't take).
- [ ] Pyodide work-function test (`dcp_local_job_test.py`) re-run to
      confirm the actual fix resolves the original blocker end-to-end.

## Notes as we go

- [x] Backed up the currently-installed, working `pythonmonkey.pyd` and
  `mozjs-136a1.dll` from site-packages to `*.orig-backup` alongside them —
  if this build goes sideways, the JS-work-function success from earlier
  this session stays reproducible without a rebuild.
- [x] One-line fix applied to `src/modules/pythonmonkey/pythonmonkey.cc`
  (confirmed exact location by reading it, matches what GitHub showed):
  added `creationOptions.setSharedMemoryAndAtomicsEnabled(true);` right
  after `JS::RealmCreationOptions creationOptions = JS::RealmCreationOptions();`
  (line ~596).
- **Hit and fixed 3 Windows-specific `setup.sh` issues, none related to
  the actual fix, all local/environmental**:
  1. `$OSTYPE` on this machine (both Git Bash and MSYS2's bash, invoked
     via `subprocess.Popen(..., shell=True)` → `cmd.exe /c bash ...`)
     reports `"cygwin"`, not `"msys"*` as the script's Windows-detection
     assumes (likely an `MSYSTEM` env var difference from not going
     through MSYS2's normal launcher). Fixed by patching all 5 `"msys"*`
     checks in `setup.sh` to also accept `"cygwin"*` (backed up as
     `setup.sh.orig-backup`).
  2. The script's own Poetry install (`curl ... | python3 - --version
     1.7.1`) calls `python3` specifically, which doesn't exist on this
     machine (only `python`) — `python3` is a Windows App Execution Alias
     stub that just prints a Microsoft Store prompt and exits non-zero.
     Confirmed Poetry is only actually *used* later in a
     `.git/hooks/pre-commit`-gated dev-tooling branch that doesn't apply
     to our shallow clone — skipped the whole Poetry install block.
  3. **A real stall, not a fast failure**: the first `wget -c` download of
     the Firefox source zip appeared to hang indefinitely — file size
     stopped growing (stuck at exactly 1,149,034,545 bytes) for 19+
     minutes, with the `bash ./setup.sh` process still alive/responding
     but with **zero child processes** (confirmed via
     `Get-CimInstance Win32_Process` walking the actual process tree:
     `python.exe` → `cmd.exe /c "bash ./setup.sh"` → `bash.exe`, no wget
     anywhere in the whole system). Killed the process tree (`TaskStop` on
     the tracked background task) and relaunched — `wget -c`'s resume
     support meant no data was lost. On relaunch, the download actually
     turned out to have already fully completed (reached "Done downloading
     spidermonkey source code" — so 1.1GB compressed is apparently the
     real final size of this pinned Firefox source snapshot, not a partial
     download after all; the first run's *true* problem is unconfirmed —
     could have been a slow/stalled final TCP segment, unclear). This
     surfaced a **second** problem: `unzip`, run non-interactively via
     Python's `subprocess.Popen` (no attached stdin), hit a
     `replace .../.arcconfig? [y/n/A/N/r]` conflict prompt against files
     left over from the first (killed) run's partial extraction, got EOF
     on stdin, defaulted to "[N]one" (skip all conflicts), and the
     subsequent `mv firefox-<rev> firefox-source` step then failed to find
     a directory to rename (exact mechanism of why unzip "succeeded"
     despite skipping everything not fully confirmed, but the practical
     fix was simple). Fixed by `rm -rf`-ing both the partial
     `firefox-<rev>` and `firefox-source` directories and rerunning — the
     zip itself didn't need to be re-fetched. **Lesson for next time**: if
     a `setup.sh` run gets killed partway through unzip, always clean up
     the extraction directories before rerunning, not just check the zip.
- **The flagged Python-version risk was real (6th issue)**: once past ATL/MFC,
  `configure` got into Mozilla's own `mozbuild` frontend (parsing
  `moz.build`/`.mozbuild` template files) and hit
  `AttributeError: module 'ast' has no attribute 'Str'` — `ast.Str` (and
  `Num`/`Bytes`/`NameConstant`/`Ellipsis`) were deprecated in Python 3.8
  and fully removed in 3.12; this machine's `python3` shim pointed at
  3.14.7. Fixed by installing Python 3.11.9 (`winget install
  Python.Python.3.11`, landed at
  `C:\Users\danie\AppData\Local\Programs\Python\Python311`) and pointing
  the `python3` shim at it instead. Note: a plain file-copy shim (which
  worked fine for 3.14) did **not** work for 3.11 — it errored with a
  missing `api-ms-win-crt-heap-l1-1-0.dll`, because standalone `python.exe`
  depends on sibling DLLs in its own install directory. Fixed by creating
  the `python3.exe` copy *inside* the Python311 directory itself (next to
  its dependencies) and adding that directory to PATH, rather than copying
  the exe out to an isolated directory like the working 3.14 shim did.
  Also had to delete Mozilla's own cached build virtualenv
  (`~/.mozbuild/srcdirs/firefox-source-<hash>/_virtualenvs`), since
  `configure` had already created and permanently bound one to the old
  3.14 interpreter on an earlier run — simply changing the shim wasn't
  enough on its own.
- 7th issue, minor, **later found to be a red herring caused by its own
  band-aid fix (see 8th issue)**: past the Python version fix, hit
  `KeyError: 'MOZILLABUILD'` from mozbuild's Visual-Studio-project-file
  generation backend (`visualstudio.py`'s `_write_mach_batch`, an optional
  convenience feature for launching `mach` from within the VS IDE — not
  needed for our command-line-only build) doing an unguarded
  `os.environ["MOZILLABUILD"]` lookup to check for an `msys2`
  subdirectory. First fix attempt: just set `MOZILLABUILD=/c/msys64` (the
  code only calls `.exists()` on the derived path, so it seemed like it
  just needed the env var to exist at all) — **this was wrong and caused
  the 8th issue below**; properly fixed there instead by guarding the
  `os.environ[...]` lookups with `.get(...)` and removing the env var.
- **8th issue: `config_sub(shell, target)` in
  `build/moz.configure/init.configure`** (line ~632) crashed with
  `TypeError: NoneType takes no arguments` inside mozbuild's own
  `shellutil._quote()` (`type(None)("'%s'")` — a type-preserving quoting
  idiom that assumes its input is `str`/`bytes`/`int`, breaks on `None`).
  Patched `shellutil.py`'s `_quote()` to special-case `None` (it's only
  used to format a human-readable `log.debug("Executing: ...")` line, not
  the actual command execution). That unmasked the real underlying issue
  one level up: `check_cmd_output(shell, config_sub, triplet)` itself
  passing `shell=None` into `subprocess.Popen`.
  **Debugging this required discovering how restrictive `.configure`
  files' execution sandbox actually is** (Mozilla's own DSL for these
  files runs them with a heavily curated set of allowed names, presumably
  to keep the config-dependency graph fully static/analyzable): plain
  `import` statements are forbidden (`ImportError: Importing modules is
  forbidden`), so is calling bare `print(...)` (`NameError: name 'print'
  is not defined`), and even referencing the builtin `Exception` class by
  name is unavailable (`NameError: name 'Exception' is not defined`) —
  but *interpreter-raised* errors (from actually executing an operation
  that fails, like an out-of-range index or a missing dict key) work fine
  and carry a real message. Landed on `{}[f"...debug info..."]` — a
  dict-lookup miss that raises a `KeyError` whose message is exactly the
  f-string given — as a reliable way to surface debug values from inside
  this sandbox without needing any disallowed name. This confirmed
  `shell=None` specifically (the target-shell `@depends` value), while
  `config_sub` (the file path) and `triplet` were both fine.
  **Turned out to be self-inflicted by the 7th issue's own fix**: patching
  just this one call site (`if shell is None: shell = ".../sh.exe"`) let
  the build get further, but the exact same `shell=None` failure then
  resurfaced in a *different* function
  (`mozillabuild_bin_paths` → `os.path.dirname(shell)` →
  `AttributeError: 'NoneType' object has no attribute 'replace'`) —
  a strong sign the real bug lived one level up, in whatever produces
  `shell` in the first place, not in each individual consumer. Read
  `shell`'s own `@depends("CONFIG_SHELL", "MOZILLABUILD")` definition
  (`init.configure` line ~137) and found it: `MOZILLABUILD=/c/msys64`
  (set for the 7th issue above) gets read here too, and this function
  tries `mozillabuild[0] + "/msys2/usr/bin/sh"` or `.../msys/bin/sh`
  depending on whether an `msys2` subfolder exists directly under
  `MOZILLABUILD` — a directory layout specific to the *official Mozilla
  Build* package (which nests a nested "msys2" folder inside itself), not
  our plain MSYS2 install (`C:\msys64`, no nested "msys2" folder, and
  using `usr/bin` not `msys/bin` anyway). Neither guessed path exists, so
  `find_program()` silently returned `None` instead of falling through to
  the correct, simpler default (bare `"sh"`, resolved via a normal PATH
  search, which would have found MSYS2's real `sh.exe` immediately).
  **Properly fixed**: reverted the band-aid in `config_sub()`, stopped
  setting `MOZILLABUILD` to a fake/misleading path entirely, and instead
  fixed the two *actual* `os.environ["MOZILLABUILD"]` call sites in
  `visualstudio.py` to use `.get("MOZILLABUILD")` with a `None`-safe
  check. This is the real fix for the 7th issue too — no env var needed at
  all, just don't crash on it being absent.
  **Lesson reinforced**: prefer fixing the actual root `@depends`
  definition (or, here, the actual root *cause* one level further back)
  over patching individual call sites one at a time — the first
  `config_sub()` patch looked like a fix but was really just relocating
  the same underlying problem to its next consumer.
  **Debugging technique note**: getting to this point required discovering
  how restrictive `.configure` files' execution sandbox is (Mozilla's own
  DSL for these files runs with a heavily curated set of allowed names,
  presumably to keep the config-dependency graph fully static/analyzable):
  plain `import` statements are forbidden (`ImportError: Importing modules
  is forbidden`), so is calling bare `print(...)` (`NameError: name
  'print' is not defined`), and even referencing the builtin `Exception`
  class by name is unavailable (`NameError: name 'Exception' is not
  defined`) — but *interpreter-raised* errors (from actually executing an
  operation that fails, like a missing dict key) work fine and carry a
  real message. `{}[f"...debug info..."]` — a dict-lookup miss raising a
  `KeyError` whose message is exactly the given f-string — is a reliable
  way to surface debug values from inside this sandbox without needing
  any disallowed name. Worth remembering if a similar issue turns up in a
  different `.configure` file.
- **9th issue**: with `configure` now **fully succeeding** (Makefiles, a
  Visual Studio solution, and a Clangd backend all generated — a real
  milestone), the subsequent `make -j$CPUS` immediately failed with
  `*** MSYS make is not supported. Stop.` (from Mozilla's own
  `config/baseconfig.mk`) — a deliberate, known check: Mozilla's build
  system rejects MSYS's own bundled `make` (known Windows path-handling
  incompatibilities between MSYS-style `/c/...` paths and native
  `C:\...` paths in GNU Make's dependency tracking) and requires a
  MinGW-w64-built `make` instead. Fixed by installing
  `pacman -S mingw-w64-x86_64-make` (landed at
  `/c/msys64/mingw64/bin/mingw32-make.exe`, GNU Make 4.4.1 "Built for
  x86_64-w64-mingw32" — confirmed the right variant despite the legacy
  "mingw32" name) and creating a `make.exe` copy *in that same directory*
  (same DLL-sibling-dependency reasoning as the Python 3.11 shim earlier —
  copying out to an isolated folder would likely break it), then
  prioritizing `/c/msys64/mingw64/bin` ahead of `/c/msys64/usr/bin` in
  PATH so plain `make` (as `setup.sh` calls it) resolves to this one
  instead of MSYS's rejected one.
- Disk space checked: 142GB free on C: before starting. Realistic total
  footprint (zip + unpacked source + build objects) estimated at
  10-15GB — not a concern.
- Extracting/deleting the Firefox source tree is itself slow on this
  machine — a plain `ls`/`du` over the partially-extracted directory
  didn't finish within a 120s tool timeout, and the `rm -rf` cleanup was
  run in the background rather than assumed instant. Expect any
  filesystem-heavy step over this source tree (extraction, `rm -rf`,
  `configure`'s own file scanning) to be slower than on Linux/macOS —
  budget real wall-clock time for these, not just the actual compile.
- 4th issue hit: once past the download/extraction (both now confirmed
  working and cached — no need to redo them), Mozilla's own `js/src`
  `configure` step calls `python3` internally too (a very common Mozilla
  build-script convention), hitting the exact same Windows Store
  App-Execution-Alias stub as `setup.sh`'s own Poetry install did earlier
  — except this one couldn't just be skipped, since it's Mozilla's build
  system, not ours. Fixed properly this time (rather than working around
  the one call site) by creating a real `python3.exe` — a straight copy of
  the working `python.exe` — at `C:\Users\danie\bin\python3.exe`, a
  directory already early in PATH (confirmed via
  `cmd.exe /c "where python3"` that this resolves before the WindowsApps
  alias stub). This should cover any other internal `python3` calls
  Mozilla's build makes too, not just the one that surfaced first.
- Also made `setup.sh`'s Firefox-source download/extract step idempotent
  (skip entirely if `firefox-source` already exists) — it wasn't safe to
  re-run originally (always re-extracted and re-`mv`d, failing with
  "Directory not empty" once a prior attempt had already succeeded at that
  step), and given how many *unrelated* environment issues we were finding
  one at a time, needing a slow re-extract on every single retry would
  have been a large, avoidable time cost.
- **ATL/MFC saga (5th issue, took several attempts)**: `configure` reached
  much further this time (compiler detection, Windows SDK, Universal CRT
  SDK all found correctly, using standalone LLVM's `clang-cl.exe` directly
  — see note below) before hitting
  `ERROR: Cannot find the ATL/MFC headers`. Three distinct problems
  stacked on top of each other before this was actually resolved:
  1. First attempt used the generic/"latest" component aliases
     (`Microsoft.VisualStudio.Component.VC.ATL` /
     `...VC.ATLMFC`) — these exist in the catalog and are real component
     IDs, but apparently target a different (likely older, "latest
     stable") MSVC toolset than the one actually installed and in use
     here (`14.51`, matching the exact toolset version named in the
     configure error's own path). Silently no-op'd — installer reported
     exit code 0 but never actually installed anything (confirmed: no
     `atlmfc` directory appeared). Found the correct, exact,
     version-matched IDs (`Microsoft.VisualStudio.Component.VC.14.51.ATL`
     / `...VC.14.51.MFC`) by grepping the VS Installer's own package
     catalog JSON (`C:\ProgramData\Microsoft\VisualStudio\Packages\_Channels\*\catalog.json`)
     for `Component.VC.*ATL`/`MFC` entries — this catalog is the
     authoritative source of truth for what component IDs actually exist
     for this specific VS release, better than guessing from generic
     naming conventions across VS versions.
  2. Retrying with the correct, version-matched IDs still failed
     (`ExitCode: 5007`, no clear message) — turned out to be a silly but
     real bug in *this session's own* PowerShell command: a stray literal
     `"--wait"` string left in the `-ArgumentList` array (confused with
     PowerShell's own, separate `-Wait` switch parameter), which
     `setup.exe modify` doesn't recognize as a valid option and rejected
     the entire command before processing any `--add` components.
  3. With that fixed, still failed (exit code still non-zero) — checked
     `vs_installer`'s own detailed log
     (`%TEMP%\dd_installer_<timestamp>.log`, much more useful than the
     bare exit code) and found the real cause:
     `"Commands with --quiet or --passive should be run elevated from the
     beginning."` — this automation session isn't running as
     Administrator (`[Security.Principal.WindowsPrincipal]::IsInRole(...Administrator)`
     confirmed `False`), and VS component installs in quiet/passive mode
     require real elevation — there's no way around this from an
     unelevated process. **Resolved by retrying with
     `Start-Process -Verb RunAs`**, which either triggered a UAC prompt
     the user approved, or Windows auto-elevated it — either way, this
     finally installed successfully and `atlmfc` now exists on disk.
  **Side finding worth flagging**: the *original* attempt to install the
  "C++ Clang Compiler for Windows" VS component (`VC.Llvm.Clang`, much
  earlier in this log) almost certainly hit this exact same silent
  elevation failure too (same non-admin session, same quiet-mode
  install) — but it didn't matter, because standalone LLVM (installed
  separately via `winget install LLVM.LLVM`, a user-level install not
  needing elevation) already provides its own fully-functional
  `clang-cl.exe`, which is what `configure` is actually finding and using
  (confirmed: `checking for the target C compiler...
  C:/PROGRA~1/LLVM/bin/clang-cl.exe`, not a path under
  `VC\Tools\Llvm\`). The VS-integrated ClangCL component may never have
  actually been installed this whole time, without it mattering.
