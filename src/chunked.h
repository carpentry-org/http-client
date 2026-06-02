#ifndef CARP_CHUNKED_H
#define CARP_CHUNKED_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

/* Maximum chunk size: 16 MiB.  Prevents malicious servers from triggering
 * unbounded allocations via an enormous chunk-size line. */
#define CHUNKED_MAX_CHUNK_SIZE (16 * 1024 * 1024)

/* Decode one chunk from a chunked transfer-encoding buffer.
 *
 * Input: buf contains raw chunked data (e.g. "1a\r\n<data>\r\n...")
 * Output: decoded chunk data written to *out, bytes consumed written to *consumed.
 *
 * Returns:
 *   1  = decoded a chunk (data in *out, length in *consumed)
 *   0  = need more data (incomplete chunk in buffer)
 *  -1  = end of stream (chunk size 0)
 *  -2  = parse error (invalid hex, overflow, or chunk too large)
 */
int chunked_decode_one(const char *buf, int buf_len, String *out, int *consumed) {
  /* Find \r\n to read chunk size */
  const char *crlf = NULL;
  for (int i = 0; i < buf_len - 1; i++) {
    if (buf[i] == '\r' && buf[i + 1] == '\n') {
      crlf = buf + i;
      break;
    }
  }
  if (!crlf) return 0; /* need more data */

  /* The chunk-size line must start with a hex digit (RFC 7230 §4.1). */
  if (buf == crlf || !isxdigit((unsigned char)buf[0])) return -2;

  /* Parse hex chunk size, validating that strtol consumed meaningful input
   * and stopped at an expected delimiter (\r for end-of-size, or ; for a
   * chunk extension per RFC 7230). */
  char *endptr = NULL;
  long chunk_size = strtol(buf, &endptr, 16);
  if (endptr == buf || (endptr != (char *)crlf && *endptr != ';')) return -2;

  /* Reject negative values (sign prefix) and sizes above the safety cap. */
  if (chunk_size < 0 || chunk_size > CHUNKED_MAX_CHUNK_SIZE) return -2;

  if (chunk_size == 0) {
    *consumed = (int)(crlf - buf) + 2; /* skip "0\r\n" */
    *out = CARP_MALLOC(1);
    (*out)[0] = '\0';
    return -1; /* end of stream */
  }

  int header_len = (int)(crlf - buf) + 2; /* "1a\r\n" */

  /* Guard against int overflow in the total-needed calculation. */
  if (chunk_size > INT_MAX - header_len - 2) return -2;
  int needed = header_len + (int)chunk_size + 2; /* +2 for trailing \r\n */

  if (buf_len < needed) return 0; /* need more data */

  /* Extract chunk data */
  *out = CARP_MALLOC(chunk_size + 1);
  memcpy(*out, buf + header_len, chunk_size);
  (*out)[chunk_size] = '\0';
  *consumed = needed;
  return 1;
}

#endif
