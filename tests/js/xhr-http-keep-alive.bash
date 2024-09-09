#! /bin/bash
#
# @file        xhr-http-keep-alive.bash
#              For testing HTTP-Keep-Alive automatically in the CI.
#
#              We use `strace` to track system calls within the process that open a TCP connection.
#              If HTTP-Keep-Alive is working, the total number of TCP connections opened should be 1 for a single remote host.
#
# @author      Tom Tang (xmader@distributive.network)
# @date        May 2024

set -u
set -o pipefail

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

cd `dirname "$0"` || panic "could not change to test directory"

if [[ "$OSTYPE" != "linux-gnu"* ]]; then
  exit 0
  # Skip non-Linux for this test
  # TODO: add tests on Windows and macOS. What's the equivalence of `strace`?
fi

code='
function newRequest(url) {
  return new Promise(function (resolve, reject) 
  {
    let xhr = new XMLHttpRequest();
    xhr.open("GET", url);
    xhr.onload = function () 
    {
      if (this.status >= 200 && this.status < 300) resolve(this.response);
      else reject(new Error(this.status));
    };
    xhr.onerror = (ev) => reject(ev.error);
    xhr.send();
  });
}

async function main() {
  await newRequest("http://www.example.org/");
  await newRequest("http://www.example.org/");
  await newRequest("http://http.badssl.com/");
}

main();
'

# Trace the `socket` system call https://man.archlinux.org/man/socket.2
# AF_INET: IPv4, IPPROTO_TCP: TCP connection
TRACE=$(strace -f -e socket \
        "${PMJS:-pmjs}" -e "$code" \
        < /dev/null 2>&1
)

# We have 3 HTTP requests, but we should only have 2 TCP connections open,
# as HTTP-Keep-Alive reuses the socket for a single remote host.
echo "$TRACE" \
| tr -d '\r' \
| grep -c -E 'socket\(AF_INET, \w*(\|\w*)+, IPPROTO_TCP\)' \
| while read qty
  do
    echo "$TRACE"
    echo "TCP connections opened: $qty"
    [ "$qty" != "2" ] && panic qty should not be $qty
    break
  done || exit $?
