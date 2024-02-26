#! /bin/bash
#
# @file         pmjs-global-arguments.bash
#               A peter-jr test which ensures that the free variable arguments in the program module's
#               context is equivalent to the pmjs argv.
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

argc=0
"${PMJS:-pmjs}" program.js abc easy as one two three |\
tr -d '\r' |\
while read keyword rest
do
  case "$keyword" in
    "ARG")
      argc=$[${argc} + 1]
      echo "${argc} ${keyword} ${rest}"
      ;;
    "ARGC")
      echo "${keyword} ${rest}"
      if [ "${rest}" != "${argc}" ]; then
        panic "program reported argc=${rest} but we only counted ${argc} arguments"
      fi
      ;;
     "FINISHED")
        if [ "${argc}" -gt 2 ]; then
          echo "Done"
          exit 111
        fi
        panic "Found ${argc} arguments, but seems like too few"
      ;;
    *)
      echo "Ignored: ${keyword} ${rest} (${argc})"
      ;;
  esac
done

exitCode="$?"
[ "$exitCode" = "111" ] && exit 0
[ "$exitCode" = "2" ] && exit 2

panic "Test did not run to completion"
