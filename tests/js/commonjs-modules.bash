#! /bin/bash
#
# @file         commonjs-modules.bash
#               A peter-jr test which runs the CommonJS Modules/1.0 test suite. Any failure in the
#               suite causes this test to fail.
# @author       Wes Garland, wes@distributive.network
# @date         June 2023

panic()
{
  echo "$*" >&2
  exit 2
}
cd `dirname "$0"`/../commonjs-official/tests/modules/1.0 || panic "could not change to test directory"

runTest()
{
  PMJS_PATH="`pwd`" ../../../../../../pmjs -e 'print=python.print' program.js\
  | while read word rest
    do
      case "$word" in
      "PASS")
        echo "$word $rest"
        return 0
      ;;
      "FAIL")
        echo "$word $rest" >&2
        return 1
      ;;
      *)
        echo "$word $rest"
      ;;
      esac
    done
}

find . -name program.js \
| while read program
  do
    testDir=`dirname "${program}"`
    cd "${testDir}"
    runTest || failures=$[${failures:-0} + 1]
    cd ..
    (exit ${failures-0})
  done
failures="$?"
echo "Done; failures: $failures"
[ "${failures}" = 0 ]
