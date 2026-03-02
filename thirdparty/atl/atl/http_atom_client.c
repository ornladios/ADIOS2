/*
 * http_atom_client.c - Hand-rolled HTTP client for ATL atom server operations.
 *
 * Uses raw sockets (no libcurl dependency).  When ATOM_SERVER_HOST
 * begins with "http://", the HTTP path is used instead of the
 * original TCP/UDP protocol.
 *
 * Optimizations:
 *   - Persistent connection with HTTP/1.1 keep-alive
 *   - Fire-and-forget POSTs for atom registration (no response wait)
 *   - Non-blocking drain of pending responses before synchronous GETs
 *
 * Talks to the korvo_server CGI endpoint:
 *   POST /v1/atoms                  - associate string<->atom
 *   GET  /v1/atoms/by-string/<str>  - string to atom
 *   GET  /v1/atoms/by-value/<int>   - atom to string
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifndef HAVE_WINDOWS_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
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

#include "atl.h"
typedef int atom_t;
#include "atom_internal.h"

/* Parsed URL components */
char *atl_http_server_url = NULL;

static char http_host[256] = "";
static int http_port = 80;
static char http_base_path[512] = "";
static int url_parsed = 0;

/* Persistent connection */
static SOCKET persistent_sock = INVALID_SOCKET;
static int pending_responses = 0;  /* number of POST responses not yet read */

static int
parse_url(const char *url)
{
    const char *host_start;
    const char *host_end;
    const char *port_str;
    int len;

    if (url_parsed) return 1;
    if (strncmp(url, "http://", 7) != 0) return 0;
    host_start = url + 7;

    port_str = strchr(host_start, ':');
    host_end = strchr(host_start, '/');

    if (port_str && (!host_end || port_str < host_end)) {
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

    if (host_end) {
        strncpy(http_base_path, host_end, sizeof(http_base_path) - 1);
        http_base_path[sizeof(http_base_path) - 1] = '\0';
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
 * Returns socket fd or -1 on error.
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
            pending_responses = 0;
        }
    }
    if (persistent_sock == INVALID_SOCKET) {
        persistent_sock = http_connect_new();
        pending_responses = 0;
    }
    return persistent_sock;
}

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
 * Drain one HTTP response from the persistent connection.
 * Handles both Content-Length and chunked Transfer-Encoding.
 * Returns status code, or -1 on error.
 */
static int
drain_one_response(SOCKET sock)
{
    char hdrbuf[4096];
    int hdr_len = 0;
    char *body_start = NULL;
    int content_length = -1;
    int is_chunked = 0;
    int n;

    /* Read until we find \r\n\r\n (end of headers) */
    while (hdr_len < (int)sizeof(hdrbuf) - 1) {
        n = sock_read(sock, hdrbuf + hdr_len, 1);
        if (n <= 0) return -1;
        hdr_len++;
        hdrbuf[hdr_len] = '\0';
        body_start = strstr(hdrbuf, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
            break;
        }
    }
    if (!body_start) return -1;

    /* Parse Content-Length from headers */
    {
        char *cl = strstr(hdrbuf, "Content-Length:");
        if (!cl) cl = strstr(hdrbuf, "content-length:");
        if (cl) {
            cl += 15;
            while (*cl == ' ') cl++;
            content_length = atoi(cl);
        }
    }

    /* Check for chunked Transfer-Encoding */
    {
        char *te = strstr(hdrbuf, "Transfer-Encoding:");
        if (!te) te = strstr(hdrbuf, "transfer-encoding:");
        if (te && strstr(te, "chunked")) {
            is_chunked = 1;
        }
    }

    if (is_chunked) {
        /* Drain chunked body */
        char line[64];
        for (;;) {
            int line_len = read_line(sock, line, sizeof(line));
            if (line_len <= 0) return -1;
            line[line_len] = '\0';
            int chunk_size = (int)strtol(line, NULL, 16);
            if (chunk_size == 0) {
                read_line(sock, line, sizeof(line));
                break;
            }
            int got = 0;
            char discard[1024];
            while (got < chunk_size) {
                int to_read = (chunk_size - got) < (int)sizeof(discard) ? (chunk_size - got) : (int)sizeof(discard);
                n = sock_read(sock, discard, to_read);
                if (n <= 0) return -1;
                got += n;
            }
            read_line(sock, line, sizeof(line));
        }
    } else if (content_length >= 0) {
        int body_already = hdr_len - (int)(body_start - hdrbuf);
        int body_remaining = content_length - body_already;
        while (body_remaining > 0) {
            char discard[1024];
            int to_read = body_remaining < (int)sizeof(discard) ? body_remaining : (int)sizeof(discard);
            n = sock_read(sock, discard, to_read);
            if (n <= 0) return -1;
            body_remaining -= n;
        }
    }

    /* Parse status code */
    {
        char *sp = strchr(hdrbuf, ' ');
        if (sp) return atoi(sp + 1);
    }
    return -1;
}

/*
 * Drain all pending POST responses (fire-and-forget cleanup).
 * Non-blocking: only drains what's available.
 */
static void
drain_pending(void)
{
    while (pending_responses > 0) {
        struct pollfd pfd;
        pfd.fd = persistent_sock;
        pfd.events = POLLIN;
        if (sock_poll(&pfd, 1, 0) <= 0) break;  /* nothing ready */
        if (pfd.revents & (POLLHUP | POLLERR)) {
            closesocket(persistent_sock);
            persistent_sock = INVALID_SOCKET;
            pending_responses = 0;
            return;
        }
        if (drain_one_response(persistent_sock) < 0) {
            closesocket(persistent_sock);
            persistent_sock = INVALID_SOCKET;
            pending_responses = 0;
            return;
        }
        pending_responses--;
    }
}

/*
 * Drain ALL pending responses, blocking until done.
 * Must be called before a synchronous GET.
 */
static void
drain_all_pending(void)
{
    while (pending_responses > 0) {
        if (drain_one_response(persistent_sock) < 0) {
            closesocket(persistent_sock);
            persistent_sock = INVALID_SOCKET;
            pending_responses = 0;
            return;
        }
        pending_responses--;
    }
}

/*
 * Read chunked transfer-encoded body from socket.
 * Appends decoded body to buf starting at *total, growing as needed.
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
            read_line(sock, line, sizeof(line));  /* trailing \r\n */
            break;
        }
        if (*total + chunk_size + 1 >= *capacity) {
            *capacity = *total + chunk_size + 256;
            *buf = realloc(*buf, *capacity);
        }
        int got = 0;
        while (got < chunk_size) {
            int n = sock_read(sock, *buf + *total + got, chunk_size - got);
            if (n <= 0) return -1;
            got += n;
        }
        *total += chunk_size;
        read_line(sock, line, sizeof(line));  /* trailing \r\n */
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

    int hdr_end = (int)(body_start - buf);
    int body_already = total - hdr_end;

    if (is_chunked) {
        total = hdr_end;
        if (read_chunked_body(sock, &buf, &total, &capacity) < 0) {
            buf[total] = '\0';
            *out_len = total;
            return buf;
        }
    } else if (content_length >= 0) {
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

static int
http_parse_response(char *response, char **body)
{
    int status = 0;
    char *p;

    if (strncmp(response, "HTTP/", 5) != 0) return -1;
    p = strchr(response, ' ');
    if (!p) return -1;
    status = atoi(p + 1);

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

/* URL-encode a string for use in URL path.
 * out must be large enough (3x input + 1). */
static void
url_encode(const char *str, char *out, int out_size)
{
    static const char hex[] = "0123456789abcdef";
    int i = 0, o = 0;
    while (str[i] && o < out_size - 4) {
        unsigned char c = (unsigned char)str[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~') {
            out[o++] = c;
        } else {
            out[o++] = '%';
            out[o++] = hex[(c >> 4) & 0x0f];
            out[o++] = hex[c & 0x0f];
        }
        i++;
    }
    out[o] = '\0';
}

/*
 * http_set_string_and_atom - Associate a string with an atom value via HTTP POST.
 *
 * Fire-and-forget: sends the POST but does not wait for the response.
 * Returns 1 on success (send), 0 on failure.
 */
int
http_set_string_and_atom(const char *str, atom_t atom)
{
    char json_body[1024];
    char request[4096];
    char path[2048];
    SOCKET sock;

    if (!parse_url(atl_http_server_url)) return 0;

    sock = ensure_connection();
    if (sock == INVALID_SOCKET) return 0;

    /* Drain any responses that are ready (non-blocking) */
    drain_pending();

    snprintf(json_body, sizeof(json_body),
             "{\"string\":\"%s\",\"atom\":%d}", str, atom);

    snprintf(path, sizeof(path), "%s/v1/atoms", http_base_path);
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             path, http_host, (int)strlen(json_body), json_body);

    if (http_send_all(sock, request, (int)strlen(request)) < 0) {
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        pending_responses = 0;
        return 0;
    }

    pending_responses++;
    return 1;
}

/*
 * http_atom_from_string - Get atom value for a string via HTTP GET.
 * Returns atom value, or -1 on failure.
 */
atom_t
http_atom_from_string(const char *str)
{
    char encoded_str[1024];
    char path[2048];
    char request[4096];
    char *response;
    char *body;
    int resp_len, status;
    SOCKET sock;
    const char *p;
    atom_t atom;

    if (!parse_url(atl_http_server_url)) return -1;

    sock = ensure_connection();
    if (sock == INVALID_SOCKET) return -1;

    /* Must drain all pending POST responses before a synchronous GET */
    drain_all_pending();

    url_encode(str, encoded_str, sizeof(encoded_str));
    snprintf(path, sizeof(path), "%s/v1/atoms/by-string/%s",
             http_base_path, encoded_str);
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "\r\n",
             path, http_host);

    if (http_send_all(sock, request, (int)strlen(request)) < 0) {
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        pending_responses = 0;
        return -1;
    }

    response = http_read_response_keepalive(sock, &resp_len);

    status = http_parse_response(response, &body);
    if (status != 200) {
        free(response);
        return -1;
    }

    /* Parse {"atom":NNN} */
    p = strstr(body, "\"atom\"");
    if (!p) {
        free(response);
        return -1;
    }
    p += 6;
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    atom = atoi(p);
    free(response);
    return atom;
}

/*
 * http_string_from_atom - Get string for an atom value via HTTP GET.
 * Returns malloc'd string, or NULL on failure.
 */
char *
http_string_from_atom(atom_t atom)
{
    char path[2048];
    char request[4096];
    char *response;
    char *body;
    int resp_len, status;
    SOCKET sock;
    const char *p, *start, *end;
    int len;
    char *result;

    if (!parse_url(atl_http_server_url)) return NULL;

    sock = ensure_connection();
    if (sock == INVALID_SOCKET) return NULL;

    /* Must drain all pending POST responses before a synchronous GET */
    drain_all_pending();

    snprintf(path, sizeof(path), "%s/v1/atoms/by-value/%d",
             http_base_path, atom);
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "\r\n",
             path, http_host);

    if (http_send_all(sock, request, (int)strlen(request)) < 0) {
        closesocket(persistent_sock);
        persistent_sock = INVALID_SOCKET;
        pending_responses = 0;
        return NULL;
    }

    response = http_read_response_keepalive(sock, &resp_len);

    status = http_parse_response(response, &body);
    if (status != 200) {
        free(response);
        return NULL;
    }

    /* Parse {"string":"<value>"} */
    p = strstr(body, "\"string\"");
    if (!p) {
        free(response);
        return NULL;
    }
    p += 8;
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (*p != '"') {
        free(response);
        return NULL;
    }
    p++;
    start = p;
    end = strchr(start, '"');
    if (!end) {
        free(response);
        return NULL;
    }
    len = (int)(end - start);
    result = malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    free(response);
    return result;
}
