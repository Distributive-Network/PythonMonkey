#! /bin/bash
#
# @file         set-interval.bash
#               A peter-jr test which ensures that the `setInterval` global function works properly.
#
# @author      Tom Tang (xmader@distributive.network)
# @date        April 2024

set -u
set -o pipefail

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

cd `dirname "$0"` || panic "could not change to test directory"

code='
let n = 0;
const timer = setInterval(()=> 
{
  console.log("callback called");

  n++;
  if (n >= 5) clearInterval(timer); // clearInterval should work inside the callback

  throw new Error("testing from the callback"); // timer should continue running regardless of whether the job function succeeds or not
}, 50);
'

"${PMJS:-pmjs}" \
-e "$code" \
< /dev/null 2> /dev/null \
| tr -d '\r' \
| grep -c '^callback called$' \
| while read qty
  do
    echo "callback called: $qty"
    [ "$qty" != "5" ] && panic qty should not be $qty
    break
  done || exit $?
