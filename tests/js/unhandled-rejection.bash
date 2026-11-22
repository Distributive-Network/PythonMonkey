#! /bin/bash
#
# @file         unhandled-rejection.bash
#               A rejection that never gets a handler is still reported as uncaught, and pmjs
#               exits non-zero.
# @author       Wes Garland, wes@distributive.network
# @date         Nov 2026
#
# timeout: 10

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

program="$(mktemp --suffix=.js)"
trap 'rm -f "${program}"' EXIT
echo 'Promise.reject(new Error("nobody handles this"));' > "${program}"
output="$("${PMJS:-pmjs}" "${program}" 2>&1)" && panic "pmjs exited 0"
echo "${output}" | grep -q 'Uncaught.*nobody handles this' || panic "no Uncaught report: ${output}"
echo PASS
