#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include <string.h>

static const char hex_chars[] = "0123456789abcdef";

/*
 * Encode binary data to hex string.
 * out must have room for len*2+1 bytes.
 * Returns out.
 */
static inline char *
hex_encode(const unsigned char *data, int len, char *out)
{
    int i;
    for (i = 0; i < len; i++) {
        out[i * 2] = hex_chars[(data[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex_chars[data[i] & 0x0f];
    }
    out[len * 2] = '\0';
    return out;
}

/*
 * Decode hex string to binary data.
 * out must have room for strlen(hex)/2 bytes.
 * Returns number of bytes written, or -1 on error.
 */
static inline int
hex_decode(const char *hex, unsigned char *out)
{
    int len = (int)strlen(hex);
    int i;
    if (len % 2 != 0) return -1;
    for (i = 0; i < len / 2; i++) {
        unsigned char hi, lo;
        char c = hex[i * 2];
        if (c >= '0' && c <= '9') hi = c - '0';
        else if (c >= 'a' && c <= 'f') hi = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') hi = c - 'A' + 10;
        else return -1;
        c = hex[i * 2 + 1];
        if (c >= '0' && c <= '9') lo = c - '0';
        else if (c >= 'a' && c <= 'f') lo = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') lo = c - 'A' + 10;
        else return -1;
        out[i] = (hi << 4) | lo;
    }
    return len / 2;
}

#endif /* HEX_UTILS_H */
