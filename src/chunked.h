#ifndef CARP_CHUNKED_H
#define CARP_CHUNKED_H

#include <string.h>
#include <stdlib.h>

/* Decode one chunk from a chunked transfer-encoding buffer.
 *
 * Input: buf contains raw chunked data (e.g. "1a\r\n<data>\r\n...")
 * Output: decoded chunk data written to *out, bytes consumed written to *consumed.
 *
 * Returns:
 *   1  = decoded a chunk (data in *out, length in *consumed)
 *   0  = need more data (incomplete chunk in buffer)
 *  -1  = end of stream (chunk size 0)
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

  /* Parse hex chunk size */
  long chunk_size = strtol(buf, NULL, 16);
  if (chunk_size == 0) {
    *consumed = (int)(crlf - buf) + 2; /* skip "0\r\n" */
    *out = CARP_MALLOC(1);
    (*out)[0] = '\0';
    return -1; /* end of stream */
  }

  int header_len = (int)(crlf - buf) + 2; /* "1a\r\n" */
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
