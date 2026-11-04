#!/bin/bash
# Source this file on macOS, then call `install_macos_brew_deps`.
#
# Installs cmake, pkg-config, wget, unzip, coreutils, llvm@19 and lld@19.
# On Intel Macs Homebrew no longer publishes bottles for newer macOS, so some
# formulae (openssl@4, llvm@19, lld@19) would be built from source, which is
# very slow or broken. The older Intel (macOS Sonoma) bottles are still on
# GHCR, so install those manually first.

# $1 = formula path on GHCR (e.g. llvm/19), $2 = version, $3 = keg name (e.g. llvm@19)
fetch_intel_bottle() {
  local token index digest cellar
  cellar="$(brew --cellar)"
  token=$(curl -fsSL "https://ghcr.io/token?service=ghcr.io&scope=repository:homebrew/core/$1:pull" | python3 -c 'import sys,json; print(json.load(sys.stdin)["token"])')
  index=$(curl -fsSL -H "Authorization: Bearer $token" -H "Accept: application/vnd.oci.image.index.v1+json" "https://ghcr.io/v2/homebrew/core/$1/manifests/$2")
  digest=$(echo "$index" | python3 -c 'import sys,json; print(next(m["annotations"]["sh.brew.bottle.digest"] for m in json.load(sys.stdin)["manifests"] if m["annotations"].get("org.opencontainers.image.ref.name","").endswith(".sonoma") and m["platform"]["architecture"]=="amd64"))')
  curl -fsSL -H "Authorization: Bearer $token" -o "/tmp/$3.tar.gz" "https://ghcr.io/v2/homebrew/core/$1/blobs/sha256:$digest"
  tar -xzf "/tmp/$3.tar.gz" -C "$cellar"
  mkdir -p "$(brew --prefix)/opt"
  ln -sfn "$cellar/$3/$2" "$(brew --prefix)/opt/$3"
}

install_macos_brew_deps() {
  brew update || true # allow failure
  if [[ "$(uname -m)" == "x86_64" ]]; then
    # openssl@4 (a dependency of coreutils): newest version with an Intel bottle is 4.0.2
    brew install ca-certificates
    fetch_intel_bottle openssl/4 4.0.2 openssl@4
    # llvm@19 depends on zstd
    brew install zstd
    fetch_intel_bottle llvm/19 19.1.7 llvm@19
    fetch_intel_bottle lld/19 19.1.7 lld@19
    brew install cmake pkg-config wget unzip coreutils # `coreutils` installs the `realpath` command
  else
    brew install cmake pkg-config wget unzip coreutils
    # SpiderMonkey requires clang >= 19 and lld as the host linker; Xcode's clang
    # is older and ships no lld. Versioned formulas because the unversioned
    # `llvm`/`lld` build from source for hours on some macOS versions.
    brew install llvm@19 lld@19
  fi
  # Both kegs are keg-only, so put them on PATH explicitly.
  export PATH="$(brew --prefix llvm@19)/bin:$(brew --prefix lld@19)/bin:$PATH"
}
