#!/bin/bash
set -euo pipefail
IFS=$'\n\t'


# Get number of CPU cores
CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 1)

echo "Installing dependencies"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then # Linux
  sudo apt-get update --yes
  sudo apt-get install cmake doxygen graphviz llvm g++ pkg-config m4 wget --yes
elif [[ "$OSTYPE" == "darwin"* ]]; then # macOS
  brew update
  brew install cmake doxygen graphviz pkg-config wget coreutils # `coreutils` installs the `realpath` command
elif [[ "$OSTYPE" == "msys"* ]]; then # Windows
  echo "Dependencies are not going to be installed automatically on Windows."
else
  echo "Unsupported OS"
  exit 1
fi
# Install rust compiler
curl --proto '=https' --tlsv1.2 https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.69 # force to use Rust 1.69 because 1.70 has linking issues on Windows
# Setup Poetry
curl -sSL https://install.python-poetry.org | python3 - --version "1.5.1"
poetry self add "poetry-dynamic-versioning[plugin]"
echo "Done installing dependencies"

echo "Downloading spidermonkey source code"
wget -c -q https://ftp.mozilla.org/pub/firefox/releases/102.11.0esr/source/firefox-102.11.0esr.source.tar.xz
mkdir -p firefox-source
tar xf firefox-102.11.0esr.source.tar.xz -C firefox-source --strip-components=1 # strip the root folder
echo "Done downloading spidermonkey source code"

echo "Building spidermonkey"
cd firefox-source
# making it work for both GNU and BSD (macOS) versions of sed
sed -i'' -e 's/os not in ("WINNT", "OSX", "Android")/os not in ("WINNT", "Android")/' ./build/moz.configure/pkg.configure # use pkg-config on macOS
sed -i'' -e '/"WindowsDllMain.cpp"/d' ./mozglue/misc/moz.build # https://discourse.mozilla.org/t/105671, https://bugzilla.mozilla.org/show_bug.cgi?id=1751561
sed -i'' -e '/"winheap.cpp"/d' ./memory/mozalloc/moz.build # https://bugzilla.mozilla.org/show_bug.cgi?id=1802675
sed -i'' -e 's/bool Unbox/JS_PUBLIC_API bool Unbox/g' ./js/public/Class.h           # need to manually add JS_PUBLIC_API to js::Unbox until it gets fixed in Spidermonkey
sed -i'' -e 's/bool js::Unbox/JS_PUBLIC_API bool js::Unbox/g' ./js/src/vm/JSObject.cpp  # same here
cd js/src
cp ./configure.in ./configure
chmod +x ./configure
mkdir -p _build
cd _build
mkdir -p ../../../../_spidermonkey_install/
../configure \
  --prefix=$(realpath $PWD/../../../../_spidermonkey_install) \
  --with-intl-api \
  --without-system-zlib \
  --disable-debug-symbols \
  --disable-jemalloc \
  --disable-tests \
  --enable-optimize 
make -j$CPUS
echo "Done building spidermonkey"

echo "Installing spidermonkey"
# install to ../../../../_spidermonkey_install/
make install 
cd ../../../../_spidermonkey_install/lib/
if [[ "$OSTYPE" == "darwin"* ]]; then # macOS
  # Set the `install_name` field to use RPATH instead of an absolute path
  # overrides https://hg.mozilla.org/releases/mozilla-esr102/file/89d799cb/js/src/build/Makefile.in#l83
  install_name_tool -id @rpath/$(basename ./libmozjs*) ./libmozjs* # making it work for whatever name the libmozjs dylib is called
fi
echo "Done installing spidermonkey"
