#!/usr/bin/env bash
# Runs the http-client test suite against a local Python server (two origins:
# primary on 8791, a second origin on 8792 for the cross-origin redirect test),
# so the suite never depends on the public internet. CI calls this instead of
# `carp -x test/http-client.carp`.
set -uo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
primary=8791
other=8792

python3 "$here/server.py" "$primary" &
pid1=$!
python3 "$here/server.py" "$other" &
pid2=$!

cleanup() { kill "$pid1" "$pid2" 2>/dev/null; }
trap cleanup EXIT

# wait for both ports to accept connections
for port in "$primary" "$other"; do
  for _ in $(seq 1 50); do
    if python3 -c "import socket,sys; s=socket.socket(); s.settimeout(0.2); sys.exit(0 if s.connect_ex(('127.0.0.1',$port))==0 else 1)"; then
      break
    fi
    sleep 0.1
  done
done

carp -x "$here/http-client.carp"
exit $?
