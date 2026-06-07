## http-client

An HTTP/HTTPS client for Carp with streaming support, chunked transfer-encoding,
and SSE-friendly response reading.

Built on [socket](https://github.com/carpentry-org/socket),
[http](https://github.com/carpentry-org/http), and
[tls](https://github.com/carpentry-org/tls).

## Installation

```clojure
(load "git@github.com:carpentry-org/http-client@0.2.0")
```

Requires OpenSSL for HTTPS support (via the `tls` library). Plain HTTP works
without OpenSSL.

## Usage

### GET

```clojure
(match (Client.get "https://example.com/")
  (Result.Success r) (println* (Response.body &r))
  (Result.Error e) (IO.errorln &e))
```

### POST with headers

```clojure
(match (Client.post "https://api.example.com/data"
         {@"Content-Type" [@"application/json"]}
         "{\"key\": 1}")
  (Result.Success r) (println* (Response.body &r))
  (Result.Error e) (IO.errorln &e))
```

### Custom verb

```clojure
(Client.request "PATCH" "https://api.example.com/x"
                {@"Authorization" [@"Bearer ..."]}
                "{}")
```

### Timeouts and redirect limits

Create a `RequestConfig` to set connect/read timeouts (in seconds) and a
redirect limit:

```clojure
(let [cfg (RequestConfig.init 5 10 10)]  ; 5s connect, 10s read, 10 redirects
  (match (Client.get-with-config "https://example.com/" &cfg)
    (Result.Success r) (println* (Response.body &r))
    (Result.Error e) (IO.errorln &e)))
```

All `-with-config` variants accept a `&RequestConfig` as the last argument.
A timeout of 0 or negative (the default) means no timeout. Connect-timeout
applies only to plain HTTP; HTTPS connections go through `TlsStream.connect`,
which does not support a timeout parameter.

### Streaming

For chunked or long-running responses, use `Client.request-stream` to get a
`ResponseStream` you can poll for chunks as they arrive:

```clojure
(match (Client.request-stream "POST" url headers body)
  (Result.Success stream)
    (do
      (while-do true
        (match (ResponseStream.poll &stream)
          (Maybe.Nothing) (break)
          (Maybe.Just chunk) (IO.print &chunk)))
      (ResponseStream.close stream))
  (Result.Error e) (IO.errorln &e))
```

`ResponseStream` handles `Transfer-Encoding: chunked` automatically and
implements the `poll` interface from the
[streams](https://github.com/carpentry-org/streams) library.

## API

### `Client`

| Function | Purpose |
|----------|---------|
| `Client.get url` | HTTP GET |
| `Client.post url headers body` | HTTP POST (auto-sets Content-Length) |
| `Client.put url headers body` | HTTP PUT |
| `Client.del url` | HTTP DELETE |
| `Client.head url` | HTTP HEAD |
| `Client.patch url headers body` | HTTP PATCH |
| `Client.request verb url headers body` | Generic request |
| `Client.request-with-max-redirects verb url headers body n` | Generic request with custom redirect limit |
| `Client.request-stream verb url headers body` | Returns a `ResponseStream` |
| `Client.request-stream-with-max-redirects verb url headers body n` | Streaming with custom redirect limit |
| `Client.get-with-config url config` | GET with request config |
| `Client.post-with-config url headers body config` | POST with request config |
| `Client.put-with-config url headers body config` | PUT with request config |
| `Client.del-with-config url config` | DELETE with request config |
| `Client.head-with-config url config` | HEAD with request config |
| `Client.patch-with-config url headers body config` | PATCH with request config |
| `Client.request-with-config verb url headers body config` | Generic request with request config |
| `Client.request-stream-with-config verb url headers body config` | Streaming with request config |

All return `(Result Response String)` (or `(Result ResponseStream String)` for the streaming variants).

All methods follow HTTP redirects automatically (up to `Client.default-max-redirects`,
which is 10). For 301/302/303 responses the method is changed to GET and the body is
dropped. For 307/308 responses the original method and body are preserved. Use the
`-with-max-redirects` variants to control the limit, or pass 0 to disable.

### `RequestConfig`

| Function | Purpose |
|----------|---------|
| `RequestConfig.init connect-timeout read-timeout max-redirects` | Create a config (timeouts in seconds, 0 = none) |
| `RequestConfig.default` | Config with no timeouts and 10 max redirects |

### `Connection`

A union of `(Plain TcpStream)` and `(Secure TlsStream)`. Used internally for
transport dispatch, but exposed in case you want lower-level control.

### `ResponseStream`

| Function | Purpose |
|----------|---------|
| `ResponseStream.poll stream` | Returns `(Maybe String)` — next decoded chunk, or `Nothing` when done |
| `ResponseStream.close stream` | Close the underlying connection |
| `ResponseStream.status-code stream` | The HTTP status code from the response headers |

## Notes

- All requests send `Connection: close` for predictable HTTP/1.1 behavior. No
  keep-alive support yet.
- HTTP/2 is not supported.

## Testing

```
carp -x test/http-client.carp
```

Tests hit `example.com` and `httpbin.org`.

<hr/>

Have fun!
