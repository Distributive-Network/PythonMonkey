#!/bin/bash
# Puts clang/lld 19 on PATH for macOS builds. Source it rather than running it,
# so the PATH change sticks; under GitHub Actions it also persists PATH for
# later steps.
#
# SpiderMonkey needs clang >= 19 and lld as the host linker, and Xcode's clang
# is older and ships no lld. Homebrew bottles llvm@19/lld@19 for arm64 only: on
# Intel it builds LLVM and Python from source, which takes hours and breaks
# whenever Homebrew's Python outgrows the runner's Xcode. LLVM's own release
# has an Intel macOS build of 19.x, so use that there instead.

LLVM_VERSION=19.1.7
LLVM_X64_SHA256=49405e75fbe7ad6f8139a33f59ec8c5112b75b3027405c7b92d19f4c6f02c78a

if [[ "$(uname -m)" == "arm64" ]]; then
  brew install llvm@19 lld@19
  LLVM_BIN="$(brew --prefix llvm@19)/bin:$(brew --prefix lld@19)/bin"
else
  LLVM_DIR="$HOME/.cache/pythonmonkey/LLVM-$LLVM_VERSION-macOS-X64"
  if [ ! -x "$LLVM_DIR/bin/clang" ]; then
    mkdir -p "$(dirname "$LLVM_DIR")"
    tarball="$LLVM_DIR.tar.xz"
    curl -fsSL -o "$tarball" "https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION/LLVM-$LLVM_VERSION-macOS-X64.tar.xz"
    echo "$LLVM_X64_SHA256  $tarball" | shasum -a 256 -c -
    # The full toolchain unpacks to many GB; take only what SpiderMonkey's
    # configure looks up (libclang is for bindgen). Leave out the bundled libc++
    # headers, which are newer than the system libc++ the build links against,
    # so clang uses the SDK's.
    includes=()
    for f in bin/clang bin/clang++ bin/clang-19 bin/clang-cpp bin/lld bin/ld64.lld bin/ld.lld \
             bin/llvm-ar bin/llvm-ranlib bin/llvm-nm bin/llvm-objcopy bin/llvm-objdump bin/llvm-otool \
             bin/llvm-strip bin/llvm-install-name-tool bin/llvm-profdata bin/llvm-config bin/dsymutil \
             lib/libclang.dylib 'lib/clang/*'; do
      includes+=(--include "LLVM-$LLVM_VERSION-macOS-X64/$f")
    done
    tar -xJf "$tarball" -C "$(dirname "$LLVM_DIR")" "${includes[@]}"
    rm -f "$tarball"
  fi
  LLVM_BIN="$LLVM_DIR/bin"
fi

export PATH="$LLVM_BIN:$PATH"
if [ -n "${GITHUB_ENV:-}" ]; then
  echo "PATH=$PATH" >> "$GITHUB_ENV"
fi
