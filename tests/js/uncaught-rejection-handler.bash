#! /bin/bash
#
# @file        uncaught-rejection-handler.bash
#              For testing if the actual JS error gets printed out for uncaught Promise rejections, 
#              instead of printing out a Python `Future exception was never retrieved` error message when not in pmjs
#
# @author      Tom Tang (xmader@distributive.network)
# @date        June 2024

set -u
set -o pipefail

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

cd `dirname "$0"` || panic "could not change to test directory"

code='
import asyncio
import pythonmonkey as pm

async def pythonmonkey_main():
  pm.eval("""void Promise.reject(new TypeError("abc"));""")
  await pm.wait()

asyncio.run(pythonmonkey_main())
'

OUTPUT=$(python -c "$code" \
         < /dev/null 2>&1
)

echo "$OUTPUT" \
| tr -d '\r' \
| (grep -c 'Future exception was never retrieved' || true) \
| while read qty
  do
    echo "$OUTPUT"
    [ "$qty" != "0" ] && panic "There shouldn't be a 'Future exception was never retrieved' error massage"
    break
  done || exit $?

echo "$OUTPUT" \
| tr -d '\r' \
| grep -c 'Uncaught TypeError: abc' \
| while read qty
  do
    [ "$qty" != "1" ] && panic "It should print out 'Uncaught TypeError' directly"
    break
  done || exit $?
