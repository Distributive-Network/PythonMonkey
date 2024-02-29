#! /bin/bash
#
# @file         program-exit.bash
#               A peter-jr test which shows that python.exit can cause a program to exit without 
#               reporting errors, even if there are pending timers.
#
# @author       Wes Garland, wes@distributive.network
# @date         July 2023
#
# timeout: 5

set -u

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

cd `dirname "$0"` || panic "could not change to test directory"

"${PMJS:-../../pmjs}" ./program-exit.js 2>&1 \
| while read line
  do
    panic "Unexpected output '$line'"
  done
exitCode="$?"
[ "$exitCode" = 0 ] || exit "$exitCode"

"${PMJS:-../../pmjs}" ./program-exit.js
exitCode="$?"

[ "$exitCode" = "99" ] || panic "exit code should have been 99 but was $exitCode"

exit 0
