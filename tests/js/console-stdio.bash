#! /bin/bash
#
# @file         console-stdio.bash
#               A peter-jr test which ensures that the console object uses the right file descriptors.
#
# @author       Wes Garland, wes@distributive.network
# @date         July 2023

set -u
set -o pipefail

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

cd `dirname "$0"` || panic "could not change to test directory"

"${PMJS:-pmjs}" \
-e 'console.log("stdout")' \
-e 'console.debug("stdout")' \
-e 'console.info("stdout")' \
< /dev/null \
| tr -d '\r' \
| grep -c '^stdout$' \
| while read qty
  do
    echo "stdout: $qty"
    [ "$qty" != "3" ] && panic qty should not be $qty
    break
  done || exit $?

"${PMJS:-pmjs}" \
-e 'console.error("stderr")' \
-e 'console.warn("stderr")' \
< /dev/null 2>&1 \
| tr -d '\r' \
| grep -c '^stderr$' \
| while read qty
  do
    echo "stderr: $qty"
    [ "$qty" != "2" ] && panic qty should not be $qty
    break
  done || exit $?

echo "done"
