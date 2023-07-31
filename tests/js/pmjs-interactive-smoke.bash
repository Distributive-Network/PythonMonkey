#! /bin/bash
#
# @file         pmjs-global-arguments.bash
#               A peter-jr smoke test which takes the REPL through executing a multiline
#               expression
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

rnd=$RANDOM
dotLines=0
("${PMJS:-pmjs}" -i <<EOF
function a(rnd)
{
  console.log('FINISHED', rnd)
};
b=
a;
b(${rnd});
EOF
)\
| cat -u | tr -d '\r' | while read prompt keyword rest
  do
    case "$keyword" in
      "...")
        echo "$prompt $keyword $rest"
        [ "$prompt" = ">" ] || panic "wrong prompt character"
        dotLines=$[${dotLines} + 1]
        ;;
      "FINISHED")
        echo "$prompt $keyword $rest"
        [ "${rnd}" = "${rest}" ] || panic "wrong function argument"
        if [ "${dotLines}" -gt 1 ]; then
          echo "Done"
          exit 111
        fi
        panic "Found ${dotLines} continued statements, but seems like too few"
        ;;
      *)
        echo "Ignored: ${prompt} ${keyword} ${rest} (${dotLines})"
        ;;
    esac
  done

exitCode="$?"
[ "$exitCode" = "111" ] && exit 0
[ "$exitCode" = "2" ] && exit 2

panic "Test did not run to completion"
