#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

# Get number of CPU cores
CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 1)

echo "Installing dependencies"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then # Linux
  SUDO=''
  if command -v sudo >/dev/null; then
    # sudo is present on the system, so use it
    SUDO='sudo'
  fi
  echo "Installing apt packages"
  $SUDO apt-get install --yes cmake llvm clang pkg-config m4 unzip \
    wget curl python3-dev
elif [[ "$OSTYPE" == "darwin"* ]]; then # macOS
  brew update || true # allow failure
  brew install cmake pkg-config wget unzip coreutils # `coreutils` installs the `realpath` command
  brew install lld
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then # Windows
  echo "Dependencies are not going to be installed automatically on Windows."
else
  echo "Unsupported OS"
  exit 1
fi
# Install rust compiler
# Skip if already installed: re-running rustup-init.sh here downloads and
# runs a fresh installer exe, which Defender/SmartScreen blocks on this box.
if command -v rustup >/dev/null && rustup toolchain list 2>/dev/null | grep -q '^1\.85'; then
  echo "Rust 1.85 toolchain already installed, skipping rustup-init"
else
  echo "Installing rust compiler"
  unset HOST_ABI_FLAGS
  if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then # Windows
    HOST_ABI_FLAGS=("--default-host" "$(clang --print-target-triple)")
  fi
  curl --proto '=https' --tlsv1.2 https://raw.githubusercontent.com/rust-lang/rustup/refs/tags/1.28.2/rustup-init.sh -sSf | sh -s -- -y ${HOST_ABI_FLAGS+"${HOST_ABI_FLAGS[@]}"} --default-toolchain 1.85
fi
CARGO_BIN="$HOME/.cargo/bin/cargo" # also works for Windows. On Windows this equals to %USERPROFILE%\.cargo\bin\cargo
command -v cbindgen >/dev/null || $CARGO_BIN install cbindgen
# Setup Poetry
if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then # Windows
  POETRY_BIN="$APPDATA/Python/Scripts/poetry"
else
  POETRY_BIN="$HOME/.local/bin/poetry"
fi
# Skip if already installed (same idempotency reasoning as rustup above).
# Falls back to `python` since this machine has no `python3` on PATH.
# Poetry is still needed below by the .git/hooks/pre-commit branch.
if command -v "$POETRY_BIN" >/dev/null || [ -x "$POETRY_BIN" ]; then
  echo "Poetry already installed, skipping"
else
  echo "Installing poetry"
  PYTHON_FOR_POETRY=$(command -v python3 || command -v python)
  curl -sSL https://install.python-poetry.org | "$PYTHON_FOR_POETRY" - --version "1.7.1"
  "$POETRY_BIN" self add 'poetry-dynamic-versioning[plugin]'
fi
echo "Done installing dependencies"

echo "Downloading spidermonkey source code"
# Read the commit hash for mozilla-central from the `mozcentral.version` file
MOZCENTRAL_VERSION=$(cat mozcentral.version)
# Skip if already extracted -- lets this script be re-run after a later
# step fails without re-downloading/re-extracting every time.
if [ ! -d firefox-source ]; then
  # curl instead of wget: wget.exe is blocked by this machine's WDAC policy.
  curl -fsSL -o firefox-source-${MOZCENTRAL_VERSION}.zip https://github.com/mozilla-firefox/firefox/archive/${MOZCENTRAL_VERSION}.zip
  unzip -q firefox-source-${MOZCENTRAL_VERSION}.zip && mv firefox-${MOZCENTRAL_VERSION} firefox-source
else
  echo "firefox-source already exists, skipping download+extract"
fi
echo "Done downloading spidermonkey source code"

echo "Building spidermonkey"
cd firefox-source

# Apply patching
# making it work for both GNU and BSD (macOS) versions of sed
sed -i'' -e 's/os not in ("WINNT", "OSX", "Android")/os not in ("WINNT", "Android")/' ./build/moz.configure/pkg.configure # use pkg-config on macOS
sed -i'' -e 's/bool Unbox/JS_PUBLIC_API bool Unbox/g' ./js/public/Class.h           # need to manually add JS_PUBLIC_API to js::Unbox until it gets fixed in Spidermonkey
sed -i'' -e 's/bool js::Unbox/JS_PUBLIC_API bool js::Unbox/g' ./js/src/vm/JSObject.cpp  # same here
sed -i'' -e 's/shared_lib = self._pretty_path(libdef.output_path, backend_file)/shared_lib = libdef.lib_name/' ./python/mozbuild/mozbuild/backend/recursivemake.py # would generate a Makefile to install the binary files from an invalid path prefix
sed -i'' -e 's/% self._pretty_path(libdef.import_path, backend_file)/% libdef.import_name/' ./python/mozbuild/mozbuild/backend/recursivemake.py # same as above. Shall we file a bug in bugzilla?
sed -i'' -e 's/if version < Version(mac_sdk_min_version())/if False/' ./build/moz.configure/toolchain.configure # do not verify the macOS SDK version as the required version is not available on Github Actions runner
sed -i'' -e 's/return JS::GetWeakRefsEnabled() == JS::WeakRefSpecifier::Disabled/return false/' ./js/src/vm/GlobalObject.cpp # forcibly enable FinalizationRegistry
sed -i'' -e 's/return !IsIteratorHelpersEnabled()/return false/' ./js/src/vm/GlobalObject.cpp # forcibly enable iterator helpers
sed -i'' -e '/MOZ_CRASH_UNSAFE_PRINTF/,/__PRETTY_FUNCTION__);/d' ./mfbt/LinkedList.h # would crash in Debug Build: in `~LinkedList()` it should have removed all this list's elements before the list's destruction
sed -i'' -e '/MOZ_ASSERT(stackRootPtr == nullptr);/d' ./js/src/vm/JSContext.cpp # would assert false in Debug Build since we extensively use `new JS::Rooted`
sed -i'' -e 's/"-fuse-ld=ld"/"-ld64" if c_compiler.version > "14.0.0" else "-fuse-ld=ld"/' ./build/moz.configure/toolchain.configure # XCode 15 changed the linker behaviour. See https://developer.apple.com/documentation/xcode-release-notes/xcode-15-release-notes#Linking
sed -i'' -e 's/defined(XP_WIN)/defined(_WIN32)/' ./mozglue/baseprofiler/public/BaseProfilerUtils.h # this header file is introduced to js/Debug.h in https://phabricator.services.mozilla.com/D221102, but it would be compiled without XP_WIN in this building configuration
sed -i'' -e 's/os\.environ\["MOZILLABUILD"\]/os.environ.get("MOZILLABUILD", "")/g' ./python/mozbuild/mozbuild/backend/visualstudio.py # avoid KeyError: we don't use the official Mozilla Build package, so this is never set

cd js/src
mkdir -p _build
cd _build
mkdir -p ../../../../_spidermonkey_install/
../configure --target=$(clang --print-target-triple) \
  --prefix=$(realpath $PWD/../../../../_spidermonkey_install) \
  --with-intl-api \
  $(if [[ "$OSTYPE" != "msys"* && "$OSTYPE" != "cygwin"* ]]; then echo "--without-system-zlib"; fi) \
  --disable-debug-symbols \
  --disable-jemalloc \
  --disable-tests \
  $(if [[ "$OSTYPE" == "darwin"* ]]; then echo "--enable-linker=ld64"; fi) \
  --enable-optimize
# --disable-explicit-resource-management (worked around Bugzilla 1940342)
# is no longer a recognized flag -- the feature it gated has since shipped.
make -j$CPUS
echo "Done building spidermonkey"

echo "Installing spidermonkey"
# install to ../../../../_spidermonkey_install/
make install 
if [[ "$OSTYPE" == "darwin"* ]]; then # macOS
  cd ../../../../_spidermonkey_install/lib/
  # Set the `install_name` field to use RPATH instead of an absolute path
  # overrides https://hg.mozilla.org/releases/mozilla-esr102/file/89d799cb/js/src/build/Makefile.in#l83
  install_name_tool -id @rpath/$(basename ./libmozjs*) ./libmozjs* # making it work for whatever name the libmozjs dylib is called
fi
echo "Done installing spidermonkey"

# if this is being ran in the root directory of the PythonMonkey repo, then include dev configurations
if test -f .git/hooks/pre-commit; then
  # set git hooks
  ln -s -f ../../githooks/pre-commit .git/hooks/pre-commit 
  # set blame ignore file 
  git config blame.ignorerevsfile .git-blame-ignore-revs
  # install autopep8
  $POETRY_BIN run pip install autopep8
  # install uncrustify
  echo "Downloading uncrustify source code"
  wget -c -q https://github.com/uncrustify/uncrustify/archive/refs/tags/uncrustify-0.78.1.tar.gz
  mkdir -p uncrustify-source
  tar -xzf uncrustify-0.78.1.tar.gz -C uncrustify-source --strip-components=1 # strip the root folder
  echo "Done downloading uncrustify source code"

  echo "Building uncrustify"
  cd uncrustify-source
  mkdir -p build
  cd build
  if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then # Windows
    cmake ../
    cmake --build . -j$CPUS --config Release
    cp Release/uncrustify.exe ../../uncrustify.exe
  else
    cmake ../
    make -j$CPUS
    cp uncrustify ../../uncrustify
  fi
  cd ../..
  echo "Done building uncrustify"
fi
