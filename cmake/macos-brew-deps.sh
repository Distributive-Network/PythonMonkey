#!/bin/bash
# Source this file on macOS, then call `install_macos_brew_deps`.
#
# Installs cmake, pkg-config, wget, unzip, llvm@19 and lld@19.
# On Intel Macs Homebrew no longer publishes bottles for newer macOS, so some
# formulae (llvm@19, lld@19) would be built from source, which is
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
  relocate_bottle "$cellar/$3"
  mkdir -p "$(brew --prefix)/opt"
  ln -sfn "$cellar/$3/$2" "$(brew --prefix)/opt/$3"
}

# Replace the @@HOMEBREW_PREFIX@@ / @@HOMEBREW_CELLAR@@ placeholders that bottles
# ship with (normally done by `brew install` when pouring a bottle).
relocate_bottle() {
  local dir="$1" prefix cellar f old
  prefix="$(brew --prefix)"
  cellar="$(brew --cellar)"
  _reloc() { local s="${1//@@HOMEBREW_PREFIX@@/$prefix}"; echo "${s//@@HOMEBREW_CELLAR@@/$cellar}"; }
  # text files (scripts, .pc, .cmake, clang config, ...)
  while IFS= read -r f; do
    sed -i '' -e "s|@@HOMEBREW_PREFIX@@|$prefix|g" -e "s|@@HOMEBREW_CELLAR@@|$cellar|g" "$f"
  done < <(grep -rIl '@@HOMEBREW_' "$dir" 2>/dev/null || true)
  # Mach-O files: dependent libraries, install names and rpaths
  find "$dir" -type f ! -type l | while IFS= read -r f; do
    file "$f" | grep -q 'Mach-O' || continue
    chmod u+w "$f"
    otool -L "$f" | awk 'NR>1 && /@@HOMEBREW_/{print $1}' | while IFS= read -r old; do
      install_name_tool -change "$old" "$(_reloc "$old")" "$f"
    done
    old="$(otool -D "$f" 2>/dev/null | sed -n '2p' || true)"
    if [[ "$old" == *@@HOMEBREW_* ]]; then install_name_tool -id "$(_reloc "$old")" "$f"; fi
    otool -l "$f" | awk '/cmd LC_RPATH/{getline; getline; if ($2 ~ /@@HOMEBREW_/) print $2}' | while IFS= read -r old; do
      install_name_tool -rpath "$old" "$(_reloc "$old")" "$f"
    done
    codesign -f -s - "$f" 2>/dev/null || true # ad-hoc re-sign after editing
  done
}

install_macos_brew_deps() {
  brew update || true # allow failure
  if [[ "$(uname -m)" == "x86_64" ]]; then
    # llvm@19 depends on zstd
    brew install zstd
    fetch_intel_bottle llvm/19 19.1.7 llvm@19
    fetch_intel_bottle lld/19 19.1.7 lld@19
    # NOTE: `coreutils` is deliberately not installed: it depends on openssl@4,
    # which has no Intel bottle at its current version (built from source, ~10 min).
    # macOS ships `realpath` natively since 12.3.
    brew install cmake pkg-config wget unzip
  else
    brew install cmake pkg-config wget unzip
    # SpiderMonkey requires clang >= 19 and lld as the host linker; Xcode's clang
    # is older and ships no lld. Versioned formulas because the unversioned
    # `llvm`/`lld` build from source for hours on some macOS versions.
    brew install llvm@19 lld@19
  fi
  # Both kegs are keg-only, so put them on PATH explicitly.
  export PATH="$(brew --prefix llvm@19)/bin:$(brew --prefix lld@19)/bin:$PATH"
  # fail fast if the (manually installed) toolchain doesn't run
  clang --version
  ld.lld --version || true
}
