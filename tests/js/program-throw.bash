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

"${PMJS:-../../pmjs}" ./program-throw.js 2>&1 1>/dev/null \
| egrep 'hello|goodbye' \
| while read line
  do
    [[ "$line" =~ goodbye ]] && panic "found goodbye - timer fired when it shouldn't have!"
    [[ "$line" =~ hello ]] && echo "found expected '$line'" && exit 0
  done
