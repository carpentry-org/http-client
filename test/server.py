#!/usr/bin/env python3
"""Minimal local test server for the http-client suite.

Reimplements just the httpbin-style endpoints the tests exercise, plus a
basic HTML site at `/`, so the suite never touches the public internet.
Run one instance per origin: `python3 server.py <port>`. All routes are
served on every port; the cross-origin test just points at a second port.
"""

import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs, unquote

INDEX_HTML = "<!doctype html>\n<html><head><title>test</title></head><body>ok</body></html>\n"


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    # -- helpers ------------------------------------------------------------
    def _send(self, code, body=b"", ctype="text/plain; charset=utf-8", extra=None):
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        for k, v in (extra or []):
            self.send_header(k, v)
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def _redirect(self, code, location):
        self.send_response(code)
        self.send_header("Location", location)
        self.send_header("Content-Length", "0")
        self.send_header("Connection", "close")
        self.end_headers()

    def _body_bytes(self):
        n = int(self.headers.get("Content-Length", 0) or 0)
        return self.rfile.read(n) if n else b""

    def _headers_dump(self):
        return "".join(f"{k}: {v}\n" for k, v in self.headers.items())

    # -- routing ------------------------------------------------------------
    def _route(self):
        parsed = urlparse(self.path)
        path = parsed.path
        qs = parse_qs(parsed.query)

        if path == "/":
            return self._send(200, INDEX_HTML, "text/html; charset=utf-8")
        if path == "/get":
            return self._send(200, "ok")
        if path == "/delete":
            return self._send(200, "deleted")
        if path == "/headers":
            return self._send(200, self._headers_dump())
        if path == "/post":
            return self._send(200, self._body_bytes())
        if path == "/cookies":
            return self._send(200, "cookies: " + self.headers.get("Cookie", ""))

        # /redirect/<n>: n hops, landing on /get
        if path.startswith("/redirect/"):
            n = int(path.rsplit("/", 1)[1])
            return self._redirect(302, f"/redirect/{n-1}" if n > 1 else "/get")

        # /absolute-redirect/1: 302 with an absolute Location built from the
        # server's own bound port (not the client's Host header, which may
        # omit the port).
        if path.startswith("/absolute-redirect/"):
            port = self.server.server_address[1]
            return self._redirect(302, f"http://127.0.0.1:{port}/get")

        # /redirect-to?url=<url>&status_code=<code>
        if path == "/redirect-to":
            url = unquote(qs.get("url", ["/get"])[0])
            code = int(qs.get("status_code", ["302"])[0])
            return self._redirect(code, url)

        # /cookies/set/<name>/<value>: set cookie then 302 to /cookies
        if path.startswith("/cookies/set/"):
            parts = path[len("/cookies/set/"):].split("/")
            name, value = parts[0], parts[1]
            self.send_response(302)
            self.send_header("Location", "/cookies")
            self.send_header("Set-Cookie", f"{name}={value}; Path=/")
            self.send_header("Content-Length", "0")
            self.send_header("Connection", "close")
            self.end_headers()
            return

        # /delay/<n>: sleep n seconds then 200 (drives read-timeout tests)
        if path.startswith("/delay/"):
            time.sleep(min(int(path.rsplit("/", 1)[1]), 15))
            return self._send(200, "slow")

        return self._send(404, "not found")

    def do_GET(self):
        self._route()

    def do_HEAD(self):
        self._route()

    def do_POST(self):
        self._route()

    def do_DELETE(self):
        self._route()

    def log_message(self, *args):
        pass  # keep test output clean


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8791
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
