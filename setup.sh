#!/bin/bash
# @file         setup.sh
#               Setup tasks for a PythonMonkey build, including locating and building the SpiderMonkey source code.
# @author       Caleb Aikens <caleb@distributive.network>
# @date         Feb 2023

set -euo pipefail
cd `dirname "$0"`
topDir="`pwd`"

usage()
{
  cat <<EOHELP

$0 - Setup build environment for PythonMonkey
Copyright (c) 2023 Distributive. Released under the terms of MIT License.

Usage: $0 [[--options] ... ]
Where:
  --help                        Shows this help
  --with-mozilla=<dir>          Sets the location of the mozilla source repo
  -with-build=<debug|release>   Sets the build type
EOHELP
}

panic()
{
  echo "$*" >&2
  exit 3
}

while getopts "dh-:" OPTION; do
  if [ "$OPTION" = "-" ]; then
    if [[ "$OPTARG" =~ (^[a-z0-9-]+)=(.*) ]]; then
      OPTION="${BASH_REMATCH[1]}"
      OPTARG="${BASH_REMATCH[2]}"
    else
      OPTION="${OPTARG}"
      OPTARG=""
    fi
  fi
  case $OPTION in
    h|help)
      usage
      exit 1
      ;;
    "with-build")
      [ "${PYTHONMONKEY_BUILD:-}" != "${OPTARG}" ] && echo "Setting PythonMonkey build type to ${OPTARG}"
      PYTHONMONKEY_BUILD="${OPTARG}"      
      ;;
    "with-mozilla")
      eval MOZILLA=`echo "${OPTARG}"`
      if [ ! -d "${MOZILLA}" ]; then
        echo "Error: ${MOZILLA} is not a directory" >&2
        exit 2
      fi
      "${topDir}/parse-moz-environment" "${MOZILLA}" || panic "Error running ${MOZILLA}/mach"
      ;;
    *)
      echo "Unrecognized option: $OPTION"
      exit 2
      ;;
  esac
done
    
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

if [ "${MOZILLA:-}" != "" ]; then
  echo "Using spidermonkey in ${MOZILLA}"
  "${topDir}"/parse-moz-environment "${MOZILLA}" || exit $?
  if [ "${MOZCONFIG:-}" = "" ]; then
    echo "Warning: MOZCONFIG not set - not building ${MOZILLA} (you can build it later)"
  else
    echo " - building SpiderMonkey in ${MOZILLA}"
    ${MOZILLA}/mach build | grep -v '^To take your build for a test drive'
  fi
else
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
fi

echo "Done setting up PythonMonkey; you can now run ./build_script.sh"
