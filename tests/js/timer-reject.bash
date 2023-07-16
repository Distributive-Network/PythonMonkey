#! /bin/bash
#
# @file         timer-reject.bash
#               A peter-jr test which shows that unhandled rejections in timers get shown on stderr,
#               exit with status 1, and aren't delayed because of pending events.
#
# @author       Wes Garland, wes@distributive.network
# @date         July 2023
#
# timeout: 10

set -u
set -o pipefail

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

cd `dirname "$0"` || panic "could not change to test directory"

"${PMJS:-../../pmjs}" ./timer-reject.js 2>&1 1>/dev/null \
| egrep 'hello|goodbye|fire' \
| (
    read line
    if [[ "$line" =~ hello ]]; then
      echo "found expected '$line'"
    else
      panic "expected hello, found '${line}'"
    fi

    read line
    if [[ "$line" =~ Error:.goodbye ]]; then
      echo "found expected '$line'"
    else
      panic "expected Error: goodbye, found '${line}'"
    fi
  )
exitCode="$?"

if [ "${exitCode}" = "1" ]; then
  echo pass
  exit 0
fi

[ "$exitCode" = 2 ] || panic "Exit code was $exitCode"
