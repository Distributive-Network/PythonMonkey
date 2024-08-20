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
elif [[ "$OSTYPE" == "msys"* ]]; then # Windows
  echo "Dependencies are not going to be installed automatically on Windows."
else
  echo "Unsupported OS"
  exit 1
fi
# Install rust compiler
echo "Installing rust compiler"
curl --proto '=https' --tlsv1.2 https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.76
CARGO_BIN="$HOME/.cargo/bin/cargo" # also works for Windows. On Windows this equals to %USERPROFILE%\.cargo\bin\cargo
$CARGO_BIN install cbindgen
# Setup Poetry
echo "Installing poetry"
curl -sSL https://install.python-poetry.org | python3 - --version "1.7.1"
if [[ "$OSTYPE" == "msys"* ]]; then # Windows
  POETRY_BIN="$APPDATA/Python/Scripts/poetry"
else
  POETRY_BIN="$HOME/.local/bin/poetry"
fi
$POETRY_BIN self add 'poetry-dynamic-versioning[plugin]'
echo "Done installing dependencies"

echo "Downloading spidermonkey source code"
# Read the commit hash for mozilla-central from the `mozcentral.version` file
MOZCENTRAL_VERSION=$(cat mozcentral.version)
wget -c -q -O firefox-source-${MOZCENTRAL_VERSION}.zip https://hg.mozilla.org/mozilla-central/archive/${MOZCENTRAL_VERSION}.zip
unzip -q firefox-source-${MOZCENTRAL_VERSION}.zip && mv mozilla-central-${MOZCENTRAL_VERSION} firefox-source
echo "Done downloading spidermonkey source code"

echo "Building spidermonkey"
cd firefox-source
# making it work for both GNU and BSD (macOS) versions of sed
sed -i'' -e 's/os not in ("WINNT", "OSX", "Android")/os not in ("WINNT", "Android")/' ./build/moz.configure/pkg.configure # use pkg-config on macOS
sed -i'' -e '/"WindowsDllMain.cpp"/d' ./mozglue/misc/moz.build # https://discourse.mozilla.org/t/105671, https://bugzilla.mozilla.org/show_bug.cgi?id=1751561
sed -i'' -e '/"winheap.cpp"/d' ./memory/mozalloc/moz.build # https://bugzilla.mozilla.org/show_bug.cgi?id=1802675
sed -i'' -e 's/"install-name-tool"/"install_name_tool"/' ./moz.configure # `install-name-tool` does not exist, but we have `install_name_tool`
sed -i'' -e 's/bool Unbox/JS_PUBLIC_API bool Unbox/g' ./js/public/Class.h           # need to manually add JS_PUBLIC_API to js::Unbox until it gets fixed in Spidermonkey
sed -i'' -e 's/bool js::Unbox/JS_PUBLIC_API bool js::Unbox/g' ./js/src/vm/JSObject.cpp  # same here
sed -i'' -e 's/shared_lib = self._pretty_path(libdef.output_path, backend_file)/shared_lib = libdef.lib_name/' ./python/mozbuild/mozbuild/backend/recursivemake.py # would generate a Makefile to install the binary files from an invalid path prefix
sed -i'' -e 's/% self._pretty_path(libdef.import_path, backend_file)/% libdef.import_name/' ./python/mozbuild/mozbuild/backend/recursivemake.py # same as above. Shall we file a bug in bugzilla?
sed -i'' -e 's/if version < Version(mac_sdk_min_version())/if False/' ./build/moz.configure/toolchain.configure # do not verify the macOS SDK version as the required version is not available on Github Actions runner
sed -i'' -e 's/return JS::GetWeakRefsEnabled() == JS::WeakRefSpecifier::Disabled/return false/' ./js/src/vm/GlobalObject.cpp # forcibly enable FinalizationRegistry
sed -i'' -e 's/return !IsIteratorHelpersEnabled()/return false/' ./js/src/vm/GlobalObject.cpp # forcibly enable iterator helpers
sed -i'' -e '/MOZ_CRASH_UNSAFE_PRINTF/,/__PRETTY_FUNCTION__);/d' ./mfbt/LinkedList.h # would crash in Debug Build: in `~LinkedList()` it should have removed all this list's elements before the list's destruction
sed -i'' -e '/MOZ_ASSERT(stackRootPtr == nullptr);/d' ./js/src/vm/JSContext.cpp # would assert false in Debug Build since we extensively use `new JS::Rooted`
cd js/src
mkdir -p _build
cd _build
mkdir -p ../../../../_spidermonkey_install/
../configure --target=$(clang --print-target-triple) \
  --prefix=$(realpath $PWD/../../../../_spidermonkey_install) \
  --with-intl-api \
  $(if [[ "$OSTYPE" != "msys"* ]]; then echo "--without-system-zlib"; fi) \
  --disable-debug-symbols \
  --disable-jemalloc \
  --disable-tests \
  --enable-optimize 
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
  if [[ "$OSTYPE" == "msys"* ]]; then # Windows
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