#! /bin/bash
#
# @file         timers-natural-exit.bash
#               A peter-jr test which show that programs exit when the event loop becomes empty.
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

"${PMJS:-../../pmjs}" ./timers-natural-exit.js \
| egrep 'end of program|fired timer' \
| (
    read line
    [ "$line" = "end of program" ] || panic "first line read was '$line', not 'end of program'"
    read line
    [ "$line" = "fired timer" ] || panic "second line read was '$line', not 'fired timer'"
  )
