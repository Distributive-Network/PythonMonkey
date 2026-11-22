#! /bin/bash
#
# @file         xhr-unref.bash
#               A program whose only pending work is an unref()'d XMLHttpRequest exits without
#               waiting for the response; a ref()'d one is waited for.
# @author       Wes Garland, wes@distributive.network
# @date         Nov 2026
#
# timeout: 30

panic()
{
  echo "FAIL: $*" >&2
  exit 2
}

tmpdir="$(mktemp -d)"
trap 'kill ${serverPid} 2>/dev/null; rm -rf "${tmpdir}"' EXIT

# A server that answers each request after 5 seconds
python3 -c '
import http.server, sys, time
class Slow(http.server.BaseHTTPRequestHandler):
  def do_GET(self):
    time.sleep(5)
    self.send_response(200); self.end_headers(); self.wfile.write(b"late")
  def log_message(self, *args): pass
server = http.server.HTTPServer(("127.0.0.1", 0), Slow)
print(server.server_address[1], flush=True)
server.serve_forever()
' > "${tmpdir}/port" &
serverPid=$!
while [ ! -s "${tmpdir}/port" ]; do sleep 0.1; done
port="$(cat "${tmpdir}/port")"

cat > "${tmpdir}/unref.js" <<EOF
const xhr = new XMLHttpRequest();
xhr.open('GET', 'http://127.0.0.1:${port}/');
xhr.onload = () => console.log('response', xhr.responseText);
xhr.unref().send();
EOF
start=$(date +%s)
output="$("${PMJS:-pmjs}" "${tmpdir}/unref.js" 2>&1)" || panic "unref program failed: ${output}"
[ $(( $(date +%s) - start )) -lt 4 ] || panic "the unref()'d request was waited for"
echo "${output}" | grep -q response && panic "the unref()'d request's response arrived: ${output}"

sed -e 's/xhr.unref().send()/xhr.unref().ref().send()/' "${tmpdir}/unref.js" > "${tmpdir}/ref.js"
output="$("${PMJS:-pmjs}" "${tmpdir}/ref.js" 2>&1)" || panic "ref program failed: ${output}"
echo "${output}" | grep -q 'response late' || panic "the ref()'d request was not waited for: ${output}"
echo PASS
