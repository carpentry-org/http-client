## http-client

An HTTP/HTTPS client for Carp with streaming support, chunked transfer-encoding,
and SSE-friendly response reading.

Built on [socket](https://github.com/carpentry-org/socket),
[http](https://github.com/carpentry-org/http), and
[tls](https://github.com/carpentry-org/tls).

## Installation

```clojure
(load "git@github.com:carpentry-org/http-client@0.0.1")
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
| `Client.patch url headers body` | HTTP PATCH |
| `Client.request verb url headers body` | Generic request |
| `Client.request-stream verb url headers body` | Returns a `ResponseStream` |

All return `(Result Response String)` (or `(Result ResponseStream String)` for the streaming variant).

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
- HTTP redirects are not followed automatically.

## Testing

```
carp -x test/http-client.carp
```

Tests hit `example.com` and `httpbin.org`.

<hr/>

Have fun!
