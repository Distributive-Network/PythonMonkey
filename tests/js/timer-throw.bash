#! /bin/bash
#
# @file         program-throw.bash
#               A peter-jr test which shows that uncaught exceptions in the program throw, get shown
#               on stderr, cause a non-zero exit code, and aren't delayed because of pending events.
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

"${PMJS:-../../pmjs}" ./timer-throw.js 2>&1 1>/dev/null \
| egrep 'hello|goodbye' \
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
