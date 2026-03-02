/*
 * http_client.c - Hand-rolled HTTP client for FFS format server operations.
 *
 * Uses raw sockets (no libcurl dependency).  When FORMAT_SERVER_HOST
 * begins with "http://", the HTTP path is used instead of the
 * original binary TCP protocol.
 *
 * Optimizations:
 *   - Persistent connection with HTTP/1.1 keep-alive
 *   - Content-Length-based response reading (no connection close needed)
 *
 * Talks to the korvo_server CGI endpoint:
 *   POST /cgi-bin/korvo_server/v1/formats   - register format
 *   GET  /cgi-bin/korvo_server/v1/formats/X  - fetch format by server_id hex
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifndef HAVE_WINDOWS_H
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <poll.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#define sock_read(s, b, l) read(s, b, l)
#define sock_write(s, b, l) write(s, b, l)
#define sock_poll(fds, n, t) poll(fds, n, t)
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#define sock_read(s, b, l) recv(s, b, l, 0)
#define sock_write(s, b, l) send(s, b, l, 0)
#define sock_poll(fds, n, t) WSAPoll(fds, n, t)
#endif

#include "fm.h"
#include "fm_internal.h"
#include "hex_utils.h"

extern FMFormat expand_format_from_rep(format_rep rep);
extern int get_format_server_verbose(void);
extern char *get_server_rep_FMformat(FMFormat format, int *rep_length);
extern unsigned char ID_length[];

/* Parsed URL components (set once from FORMAT_SERVER_HOST) */
char *ffs_http_server_url = NULL;

static char http_host[256] = "";
static int http_port = 80;
static char http_base_path[512] = "";
static int url_parsed = 0;

/* Persistent connection */
static SOCKET persistent_sock = INVALID_SOCKET;

/*
 * Parse "http://host[:port]/path" into host, port, base_path.
 * Returns 1 on success, 0 on failure.
 */
static int
parse_http_url(const char *url)
{
    const char *host_start;
    const char *host_end;
    const char *port_str;
    int len;

    if (url_parsed) return 1;
    if (strncmp(url, "http://", 7) != 0) return 0;
    host_start = url + 7;

    /* Find end of host (: or / or end of string) */
    port_str = strchr(host_start, ':');
    host_end = strchr(host_start, '/');

    if (port_str && (!host_end || port_str < host_end)) {
        /* Have port */
        len = (int)(port_str - host_start);
        if (len >= (int)sizeof(http_host)) len = sizeof(http_host) - 1;
        memcpy(http_host, host_start, len);
        http_host[len] = '\0';
        http_port = atoi(port_str + 1);
    } else {
        if (host_end) {
            len = (int)(host_end - host_start);
        } else {
            len = (int)strlen(host_start);
        }
        if (len >= (int)sizeof(http_host)) len = sizeof(http_host) - 1;
        memcpy(http_host, host_start, len);
        http_host[len] = '\0';
        http_port = 80;
    }

    /* base_path is everything from the first / onward (or empty) */
    if (host_end) {
        strncpy(http_base_path, host_end, sizeof(http_base_path) - 1);
        http_base_path[sizeof(http_base_path) - 1] = '\0';
        /* Strip trailing slash */
        len = (int)strlen(http_base_path);
        if (len > 1 && http_base_path[len - 1] == '/') {
            http_base_path[len - 1] = '\0';
        }
    } else {
        http_base_path[0] = '\0';
    }
    url_parsed = 1;
    return 1;
}

/*
 * Open a new TCP connection to the HTTP server.
 * Returns socket or INVALID_SOCKET on error.
 */
static SOCKET
http_connect_new(void)
{
    struct sockaddr_in addr;
    struct hostent *he;
    SOCKET sock;
    int delay_value = 1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(http_port);

    he = gethostbyname(http_host);
    if (he) {
        memcpy(&addr.sin_addr, he->h_addr, he->h_length);
    } else {
        unsigned long ip = inet_addr(http_host);
        if (ip == (unsigned long)-1) {
            closesocket(sock);
            return INVALID_SOCKET;
        }
        addr.sin_addr.s_addr = ip;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
               (char *)&delay_value, sizeof(delay_value));
    return sock;
}

/*
 * Ensure the persistent connection is open.
 * Returns socket or INVALID_SOCKET on error.
 */
static SOCKET
ensure_connection(void)
{
    if (persistent_sock != INVALID_SOCKET) {
        /* Quick check if connection is still alive */
        struct pollfd pfd;
        pfd.fd = persistent_sock;
        pfd.events = POLLIN;
        if (sock_poll(&pfd, 1, 0) > 0 && (pfd.revents & (POLLHUP | POLLERR))) {
            closesocket(persistent_sock);
            persistent_sock = INVALID_SOCKET;
        }
    }
    if (persistent_sock == INVALID_SOCKET) {
        persistent_sock = http_connect_new();
    }
    return persistent_sock;
}

/*
 * Send a full buffer to socket.
 */
static int
http_send_all(SOCKET sock, const char *buf, int len)
{
    int sent = 0;
    while (sent < len) {
        int n = sock_write(sock, buf + sent, len - sent);
        if (n <= 0) return -1;
        sent += n;
    }
    return sent;
}

/*
 * Read a line from socket (up to \n).  Returns length including \n,
 * or -1 on error.  Does not null-terminate.
 */
static int
read_line(SOCKET sock, char *buf, int buf_size)
{
    int i = 0;
    while (i < buf_size - 1) {
        int n = sock_read(sock, buf + i, 1);
        if (n <= 0) return (i > 0) ? i : -1;
        i++;
        if (buf[i - 1] == '\n') break;
    }
    return i;
}

/*
 * Read chunked transfer-encoded body from socket.
 * Appends decoded body to buf starting at *total, growing as needed.
 * Updates *total and *capacity.
 */
static int
read_chunked_body(SOCKET sock, char **buf, int *total, int *capacity)
{
    char line[64];
    for (;;) {
        int line_len = read_line(sock, line, sizeof(line));
        if (line_len <= 0) return -1;
        line[line_len] = '\0';
        int chunk_size = (int)strtol(line, NULL, 16);
        if (chunk_size == 0) {
            /* Terminal chunk; read trailing \r\n */
            read_line(sock, line, sizeof(line));
            break;
        }
        /* Ensure space */
        if (*total + chunk_size + 1 >= *capacity) {
            *capacity = *total + chunk_size + 256;
            *buf = realloc(*buf, *capacity);
        }
        /* Read exactly chunk_size bytes */
        int got = 0;
        while (got < chunk_size) {
            int n = sock_read(sock, *buf + *total + got, chunk_size - got);
            if (n <= 0) return -1;
            got += n;
        }
        *total += chunk_size;
        /* Read trailing \r\n after chunk data */
        read_line(sock, line, sizeof(line));
    }
    (*buf)[*total] = '\0';
    return 0;
}

/*
 * Read a full HTTP response on the persistent (keep-alive) connection.
 * Handles both Content-Length and chunked Transfer-Encoding.
 * Returns malloc'd buffer containing full response, sets *out_len.
 */
static char *
http_read_response_keepalive(SOCKET sock, int *out_len)
{
    int capacity = 4096;
    int total = 0;
    char *buf = malloc(capacity);
    char *body_start = NULL;
    int content_length = -1;
    int is_chunked = 0;
    int n;

    /* Read byte-by-byte until we find end of headers */
    while (total < capacity - 1) {
        n = sock_read(sock, buf + total, 1);
        if (n <= 0) {
            buf[total] = '\0';
            *out_len = total;
            return buf;
        }
        total++;
        buf[total] = '\0';
        body_start = strstr(buf, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
            break;
        }
    }

    if (!body_start) {
        *out_len = total;
        return buf;
    }

    /* Parse Content-Length from headers */
    {
        char *cl = strstr(buf, "Content-Length:");
        if (!cl) cl = strstr(buf, "content-length:");
        if (cl) {
            cl += 15;
            while (*cl == ' ') cl++;
            content_length = atoi(cl);
        }
    }

    /* Check for chunked Transfer-Encoding */
    {
        char *te = strstr(buf, "Transfer-Encoding:");
        if (!te) te = strstr(buf, "transfer-encoding:");
        if (te && strstr(te, "chunked")) {
            is_chunked = 1;
        }
    }

    /* Note where body starts in buffer, then reset total to header end */
    int hdr_end = (int)(body_start - buf);
    int body_already = total - hdr_end;

    if (is_chunked) {
        /* Move any already-read body bytes to start of body area */
        total = hdr_end;  /* rewind to just after headers */
        /* Push back the bytes we already read past headers:
         * We can't un-read from the socket, so we need to handle them.
         * Simplest: copy them aside and prepend to chunk parsing. */
        /* For simplicity, since headers are read byte-by-byte, body_already
         * should be 0 in practice. But handle it safely. */
        if (body_already > 0) {
            /* Shift body bytes we've already consumed back for chunk parsing.
             * Actually these bytes are the start of the first chunk size line. */
            char prefix[64];
            int plen = body_already < (int)sizeof(prefix) ? body_already : (int)sizeof(prefix);
            memcpy(prefix, body_start, plen);
            total = hdr_end;
            /* We need to parse chunk sizes from these bytes + more from socket.
             * For simplicity, put them back in buf and handle in read_chunked_body.
             * Instead, let's just handle the common case where body_already==0. */
        }
        total = hdr_end;
        if (read_chunked_body(sock, &buf, &total, &capacity) < 0) {
            buf[total] = '\0';
            *out_len = total;
            return buf;
        }
    } else if (content_length >= 0) {
        /* Content-Length based reading */
        int body_remaining = content_length - body_already;
        while (body_remaining > 0) {
            if (total + body_remaining >= capacity) {
                capacity = total + body_remaining + 1;
                buf = realloc(buf, capacity);
            }
            n = sock_read(sock, buf + total, body_remaining);
            if (n <= 0) break;
            total += n;
            body_remaining -= n;
        }
    }

    buf[total] = '\0';
    *out_len = total;
    return buf;
}

/*
 * Parse HTTP response: extract status code and body.
 * Body starts after "\r\n\r\n".
 * Returns status code, sets *body to point within response.
 */
static int
http_parse_response(char *response, char **body)
{
    int status = 0;
    char *p;

    /* "HTTP/1.x NNN ..." */
    if (strncmp(response, "HTTP/", 5) != 0) return -1;
    p = strchr(response, ' ');
    if (!p) return -1;
    status = atoi(p + 1);

    /* Find body after blank line */
    p = strstr(response, "\r\n\r\n");
    if (p) {
        *body = p + 4;
    } else {
        p = strstr(response, "\n\n");
        if (p) {
            *body = p + 2;
        } else {
            *body = response + strlen(response);
        }
    }
    return status;
}

/*
 * Extract a JSON string value for a given key from a JSON body.
 * Returns malloc'd string, or NULL.
 */
static char *
json_extract_string(const char *json, const char *key)
{
    char pattern[256];
    const char *p, *start, *end;
    int len;
    char *result;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (!p) return NULL;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (*p != '"') return NULL;
    p++;
    start = p;
    end = strchr(start, '"');
    if (!end) return NULL;
    len = (int)(end - start);
    result = malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

/*
 * http_server_register_format - Register a format via HTTP POST.
 *
 * Sends the format_rep as hex-encoded JSON to the CGI server.
 * Receives server_id hex string, decodes it into format->server_ID.
 *
 * Returns 1 on success, 0 on failure.
 */
int
http_server_register_format(FMContext fmc, FMFormat format)
{
    int rep_len;
    char *rep_data;
    char *hex_rep;
    char *request;
    char *json_body;
    int json_len;
    char *response;
    int resp_len;
    char *body;
    int status;
    char *hex_id;
    int id_len;
    SOCKET sock;
    char path[2048];
    int request_len;

    if (!parse_http_url(ffs_http_server_url)) return 0;

    sock = ensure_connection();
    if (sock == INVALID_SOCKET) return 0;

    /* Get the binary format rep */
    rep_data = get_server_rep_FMformat(format, &rep_len);
    if (!rep_data) return 0;

    /* Hex-encode format rep */
    hex_rep = malloc(rep_len * 2 + 1);
    hex_encode((unsigned char *)rep_data, rep_len, hex_rep);

    /* Build JSON body: {"format_rep":"<hex>"} */
    json_len = (int)strlen(hex_rep) + 32;
    json_body = malloc(json_len);
    snprintf(json_body, json_len, "{\"format_rep\":\"%s\"}", hex_rep);
    free(hex_rep);
    json_len = (int)strlen(json_body);

    /* Build HTTP/1.1 request with keep-alive */
    snprintf(path, sizeof(path), "%s/v1/formats", http_base_path);
    request_len = json_len + 512;
    request = malloc(request_len);
    snprintf(request, request_len,
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             path, http_host, json_len, json_body);
    free(json_body);

    if (http_send_all(sock, request, (int)strlen(request)) < 0) {
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        free(request);
        return 0;
    }
    free(request);

    response = http_read_response_keepalive(sock, &resp_len);

    status = http_parse_response(response, &body);
    if (status != 200) {
        if (get_format_server_verbose()) {
            fprintf(stderr, "HTTP format register failed, status %d\n", status);
        }
        free(response);
        /* Connection may be bad after error */
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        return 0;
    }

    /* Parse server_id from response JSON */
    hex_id = json_extract_string(body, "server_id");
    free(response);

    if (!hex_id) {
        if (get_format_server_verbose()) {
            fprintf(stderr, "HTTP format register: no server_id in response\n");
        }
        return 0;
    }

    id_len = (int)strlen(hex_id) / 2;
    format->server_ID.length = id_len;
    format->server_ID.value = malloc(id_len);
    hex_decode(hex_id, (unsigned char *)format->server_ID.value);
    free(hex_id);

    return 1;
}

/*
 * http_server_get_format - Fetch a format by server_id via HTTP GET.
 *
 * Returns FMFormat on success, NULL on failure.
 * The returned format is added to the context via add_format_to_iofile.
 */
FMFormat
http_server_get_format(FMContext fmc, void *buffer)
{
    int id_size;
    char hex_id[256];
    char path[2048];
    char *request;
    int request_len;
    char *response;
    int resp_len;
    char *body;
    int status;
    char *hex_rep;
    unsigned char *rep_data;
    int rep_len;
    format_rep rep;
    FMFormat format;
    SOCKET sock;

    if (!parse_http_url(ffs_http_server_url)) return NULL;

    sock = ensure_connection();
    if (sock == INVALID_SOCKET) return NULL;

    /* Determine ID size from version.
     * ID_length has 3 entries for versions 0, 1, 2. */
    if (version_of_format_ID(buffer) < 3) {
        id_size = ID_length[version_of_format_ID(buffer)];
    } else {
        id_size = 8;
    }

    /* Hex-encode the server_id */
    hex_encode((unsigned char *)buffer, id_size, hex_id);

    /* Build HTTP/1.1 GET request with keep-alive */
    snprintf(path, sizeof(path), "%s/v1/formats/%s", http_base_path, hex_id);
    request_len = (int)strlen(path) + 512;
    request = malloc(request_len);
    snprintf(request, request_len,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "\r\n",
             path, http_host);

    if (http_send_all(sock, request, (int)strlen(request)) < 0) {
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        free(request);
        return NULL;
    }
    free(request);

    response = http_read_response_keepalive(sock, &resp_len);

    status = http_parse_response(response, &body);
    if (status != 200) {
        if (get_format_server_verbose()) {
            fprintf(stderr, "HTTP format get failed, status %d\n", status);
        }
        free(response);
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        return NULL;
    }

    /* Parse format_rep from response JSON */
    hex_rep = json_extract_string(body, "format_rep");
    free(response);

    if (!hex_rep) {
        if (get_format_server_verbose()) {
            fprintf(stderr, "HTTP format get: no format_rep in response\n");
        }
        return NULL;
    }

    rep_len = (int)strlen(hex_rep) / 2;
    rep_data = malloc(rep_len);
    if (hex_decode(hex_rep, rep_data) < 0) {
        free(hex_rep);
        free(rep_data);
        return NULL;
    }
    free(hex_rep);

    /* Build format_rep struct - the binary data IS the format_rep */
    rep = (format_rep)rep_data;

    format = expand_format_from_rep(rep);
    if (format == NULL) {
        return NULL;
    }

    add_format_to_iofile(fmc, format, id_size, buffer, -1);
    return format;
}
