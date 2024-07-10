#! /bin/bash
#
# @file         commonjs-modules.bash
#               A peter-jr test which runs the CommonJS Modules/1.0 test suite. Any failure in the
#               suite causes this test to fail.
# @author       Wes Garland, wes@distributive.network
# @date         June 2023
# 
# timeout: 40

panic()
{
  echo "$*" >&2
  exit 2
}

cd `dirname "$0"`
git submodule update --init --recursive || panic "could not checkout the required git submodule"

cd ../commonjs-official/tests/modules/1.0 || panic "could not change to test directory"

runTest()
{
  testName="`printf '%20s' \"$1\"`"
  set -o pipefail
  echo -n "${testName}: "

  PMJS_PATH="`pwd`" pmjs -e 'print=python.print' program.js\
  | tr -d '\r'\
  | while read word rest
    do
      case "$word" in
      "PASS"|"DONE")
        echo -n "$word $rest"
        return 0
      ;;
      "FAIL")
        echo -n "\r${testName}: $word $rest" >&2
        return 1
      ;;
      *)
        echo "$word $rest"
        echo -n "${testName}: "
      ;;
      esac
      (exit 2)
    done
  ret="$?"
  echo
  return "$ret"
}

find . -name program.js \
| while read program
  do
    testDir=`dirname "${program}"`
    cd "${testDir}"
    runTest "`basename ${testDir}`" || failures=$[${failures:-0} + 1]
    cd ..
    (exit ${failures-0})
  done
failures="$?"
echo "Done; failures: $failures"
[ "${failures}" = 0 ]
