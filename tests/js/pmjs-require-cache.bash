#! /bin/bash
#
# @file         pmjs-require-cache.bvash
#               A peter-jr test which ensures that the require cache for the extra-module environment
#               is the same as the require cache for the program module.
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

loaded=0
"${PMJS:-pmjs}" -r ./modules/print-load -r ./modules/print-load program.js |\
tr -d '\r' |\
while read keyword rest
do
  case "$keyword" in
    "LOADED")
      echo "${keyword} ${rest}"
      loaded=$[${loaded} + 1]
      [ "${loaded}" != 1 ] && panic "loaded module more than once!"
      ;;
    "FINISHED")
      echo "${keyword} ${rest}"
      if [ "${loaded}" = 1 ]; then
        echo "Done"
        exit 111
      fi
      ;;
    *)
      echo "Ignored: ${keyword} ${rest} (${loaded})"
      ;;
  esac
done

exitCode="$?"
[ "$exitCode" = "111" ] && exit 0
[ "$exitCode" = "2" ] && exit 2

panic "Test did not run to completion"
