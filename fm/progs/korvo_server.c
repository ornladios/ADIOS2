/*
 * korvo_server.c - Unified CGI server for FFS formats and ATL atoms.
 *
 * Deployed as a CGI binary behind Apache.  Apache sets environment
 * variables REQUEST_METHOD, PATH_INFO, CONTENT_LENGTH; POST body
 * comes from stdin, response goes to stdout.
 *
 * Persistence is file-based under KORVO_DATA_DIR:
 *   formats/<hex_server_id>   - raw binary format_rep
 *   atoms/s/<url-safe-string> - text: "<atom_int>\n"
 *   atoms/v/<atom_int>        - text: "<string>\n"
 *
 * REST API:
 *   POST /v1/formats                  - register format
 *   GET  /v1/formats/<hex_server_id>  - fetch format
 *   POST /v1/atoms                    - associate string<->atom
 *   GET  /v1/atoms/by-string/<str>    - string to atom
 *   GET  /v1/atoms/by-value/<int>     - atom to string
 *   GET  /v1/health                   - health check
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "fm.h"
#include "fm_internal.h"
#include "../hex_utils.h"

static const char *data_dir = "/tmp/korvo_data";

/* ----------------------------------------------------------------
 * CGI helpers
 * ---------------------------------------------------------------- */

static void
respond(int status, const char *status_text, const char *content_type,
        const char *body)
{
    printf("Status: %d %s\r\n", status, status_text);
    printf("Content-Type: %s\r\n", content_type);
    printf("Access-Control-Allow-Origin: *\r\n");
    printf("\r\n");
    if (body) {
        fputs(body, stdout);
    }
}

static void json_ok(const char *body) { respond(200, "OK", "application/json", body); }
static void json_not_found(const char *msg) { respond(404, "Not Found", "application/json", msg); }
static void json_error(const char *msg) { respond(400, "Bad Request", "application/json", msg); }
static void json_server_error(const char *msg) { respond(500, "Internal Server Error", "application/json", msg); }

static char *
read_post_body(int *out_len)
{
    const char *cl_str = getenv("CONTENT_LENGTH");
    int len;
    char *buf;
    if (!cl_str) {
        *out_len = 0;
        return NULL;
    }
    len = atoi(cl_str);
    if (len <= 0 || len > 10 * 1024 * 1024) {
        *out_len = 0;
        return NULL;
    }
    buf = malloc(len + 1);
    if (!buf) {
        *out_len = 0;
        return NULL;
    }
    if ((int)fread(buf, 1, len, stdin) != len) {
        free(buf);
        *out_len = 0;
        return NULL;
    }
    buf[len] = '\0';
    *out_len = len;
    return buf;
}

/* Simple JSON value extraction - finds "key":"value" and returns
 * malloc'd copy of value.  For integer values, finds "key":NNN. */
static char *
json_get_string(const char *json, const char *key)
{
    char pattern[256];
    const char *p, *start, *end;
    char *result;
    int len;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (!p) return NULL;
    p += strlen(pattern);
    /* skip whitespace and colon */
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (*p != '"') return NULL;
    p++; /* skip opening quote */
    start = p;
    /* find closing quote (no escape handling needed for hex strings) */
    end = strchr(start, '"');
    if (!end) return NULL;
    len = (int)(end - start);
    result = malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

static int
json_get_int(const char *json, const char *key)
{
    char pattern[256];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (!p) return -1;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    return atoi(p);
}

/* ----------------------------------------------------------------
 * File-based persistence helpers
 * ---------------------------------------------------------------- */

static void
ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir(path, 0755);
    }
}

static void
ensure_data_dirs(void)
{
    char path[1024];
    ensure_dir(data_dir);
    snprintf(path, sizeof(path), "%s/formats", data_dir);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/atoms", data_dir);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/atoms/s", data_dir);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/atoms/v", data_dir);
    ensure_dir(path);
}

/* URL-safe encoding for atom string filenames:
 * Replace / with _S_, space with _W_, and other problematic chars
 * with _XX_ where XX is hex.  This is simple and reversible. */
static void
url_safe_encode(const char *str, char *out, int out_size)
{
    int i = 0, o = 0;
    while (str[i] && o < out_size - 5) {
        unsigned char c = (unsigned char)str[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_') {
            out[o++] = c;
        } else {
            out[o++] = '%';
            out[o++] = hex_chars[(c >> 4) & 0x0f];
            out[o++] = hex_chars[c & 0x0f];
        }
        i++;
    }
    out[o] = '\0';
}

static void
url_safe_decode(const char *enc, char *out, int out_size)
{
    int i = 0, o = 0;
    while (enc[i] && o < out_size - 1) {
        if (enc[i] == '%' && enc[i+1] && enc[i+2]) {
            unsigned char buf[1];
            char hex[3] = { enc[i+1], enc[i+2], '\0' };
            if (hex_decode(hex, buf) == 1) {
                out[o++] = (char)buf[0];
                i += 3;
                continue;
            }
        }
        out[o++] = enc[i++];
    }
    out[o] = '\0';
}

/* ----------------------------------------------------------------
 * Format handlers
 * ---------------------------------------------------------------- */

static void
handle_post_format(const char *body)
{
    char *hex_rep;
    unsigned char *format_rep;
    int rep_len;
    server_ID_type server_id;
    char hex_id[256];
    char filepath[1024];
    char response[512];
    FILE *f;

    hex_rep = json_get_string(body, "format_rep");
    if (!hex_rep) {
        json_error("{\"error\":\"missing format_rep\"}");
        return;
    }

    rep_len = (int)strlen(hex_rep) / 2;
    if (rep_len < (int)sizeof(struct _format_wire_format_1)) {
        free(hex_rep);
        json_error("{\"error\":\"format_rep too short\"}");
        return;
    }
    format_rep = malloc(rep_len);
    if (hex_decode(hex_rep, format_rep) < 0) {
        free(hex_rep);
        free(format_rep);
        json_error("{\"error\":\"invalid hex in format_rep\"}");
        return;
    }
    free(hex_rep);

    /* Validate that format_rep_length is consistent */
    {
        struct _format_wire_format_1 *hdr = (struct _format_wire_format_1 *)format_rep;
        int declared_len = ntohs(hdr->format_rep_length);
        if (hdr->server_rep_version > 0) {
            declared_len += (ntohs(hdr->top_bytes_format_rep_length) << 16);
        }
        if (declared_len > rep_len) {
            free(format_rep);
            json_error("{\"error\":\"format_rep_length mismatch\"}");
            return;
        }
    }

    /* Generate server ID from format rep (same as format_server) */
    generate_format3_server_ID(&server_id,
                               (struct _format_wire_format_1 *)format_rep);

    hex_encode((unsigned char *)server_id.value, server_id.length, hex_id);

    /* Write format_rep to file (if not already present) */
    snprintf(filepath, sizeof(filepath), "%s/formats/%s", data_dir, hex_id);
    f = fopen(filepath, "rb");
    if (f) {
        /* Already exists - that's fine, same hash means same content */
        fclose(f);
    } else {
        f = fopen(filepath, "wb");
        if (!f) {
            free(format_rep);
            free(server_id.value);
            json_server_error("{\"error\":\"cannot write format file\"}");
            return;
        }
        fwrite(format_rep, 1, rep_len, f);
        fclose(f);
    }

    free(format_rep);
    free(server_id.value);

    snprintf(response, sizeof(response),
             "{\"server_id\":\"%s\"}", hex_id);
    json_ok(response);
}

static void
handle_get_format(const char *hex_id)
{
    char filepath[1024];
    FILE *f;
    unsigned char *buf;
    long file_len;
    char *hex_rep;
    char *response;

    snprintf(filepath, sizeof(filepath), "%s/formats/%s", data_dir, hex_id);
    f = fopen(filepath, "rb");
    if (!f) {
        json_not_found("{\"error\":\"format not found\"}");
        return;
    }
    fseek(f, 0, SEEK_END);
    file_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(file_len);
    if ((long)fread(buf, 1, file_len, f) != file_len) {
        fclose(f);
        free(buf);
        json_server_error("{\"error\":\"read error\"}");
        return;
    }
    fclose(f);

    hex_rep = malloc(file_len * 2 + 1);
    hex_encode(buf, (int)file_len, hex_rep);
    free(buf);

    /* Build response: {"format_rep":"<hex>"} */
    response = malloc(file_len * 2 + 64);
    snprintf(response, file_len * 2 + 64,
             "{\"format_rep\":\"%s\"}", hex_rep);
    free(hex_rep);
    json_ok(response);
    free(response);
}

/* ----------------------------------------------------------------
 * Atom handlers
 * ---------------------------------------------------------------- */

static void
handle_post_atom(const char *body)
{
    char *str;
    int atom;
    char safe_str[512];
    char filepath[1024];
    FILE *f;

    str = json_get_string(body, "string");
    atom = json_get_int(body, "atom");
    if (!str || atom < 0) {
        if (str) free(str);
        json_error("{\"error\":\"missing string or atom\"}");
        return;
    }

    ensure_data_dirs();
    url_safe_encode(str, safe_str, sizeof(safe_str));

    /* Write string->atom mapping */
    snprintf(filepath, sizeof(filepath), "%s/atoms/s/%s", data_dir, safe_str);
    f = fopen(filepath, "w");
    if (!f) {
        free(str);
        json_server_error("{\"error\":\"cannot write atom file\"}");
        return;
    }
    fprintf(f, "%d\n", atom);
    fclose(f);

    /* Write atom->string mapping */
    snprintf(filepath, sizeof(filepath), "%s/atoms/v/%d", data_dir, atom);
    f = fopen(filepath, "w");
    if (!f) {
        free(str);
        json_server_error("{\"error\":\"cannot write atom value file\"}");
        return;
    }
    fprintf(f, "%s\n", str);
    fclose(f);

    free(str);
    json_ok("{\"ok\":true}");
}

static void
handle_get_atom_by_string(const char *str)
{
    char safe_str[512];
    char filepath[1024];
    FILE *f;
    int atom;
    char response[256];

    url_safe_encode(str, safe_str, sizeof(safe_str));
    snprintf(filepath, sizeof(filepath), "%s/atoms/s/%s", data_dir, safe_str);
    f = fopen(filepath, "r");
    if (!f) {
        json_not_found("{\"error\":\"atom not found\"}");
        return;
    }
    if (fscanf(f, "%d", &atom) != 1) {
        fclose(f);
        json_server_error("{\"error\":\"corrupt atom file\"}");
        return;
    }
    fclose(f);
    snprintf(response, sizeof(response), "{\"atom\":%d}", atom);
    json_ok(response);
}

static void
handle_get_atom_by_value(int atom)
{
    char filepath[1024];
    FILE *f;
    char str[512];
    char response[1024];
    int len;

    snprintf(filepath, sizeof(filepath), "%s/atoms/v/%d", data_dir, atom);
    f = fopen(filepath, "r");
    if (!f) {
        json_not_found("{\"error\":\"atom value not found\"}");
        return;
    }
    if (!fgets(str, sizeof(str), f)) {
        fclose(f);
        json_server_error("{\"error\":\"corrupt atom value file\"}");
        return;
    }
    fclose(f);
    /* Strip trailing newline */
    len = (int)strlen(str);
    if (len > 0 && str[len - 1] == '\n') str[len - 1] = '\0';

    snprintf(response, sizeof(response), "{\"string\":\"%s\"}", str);
    json_ok(response);
}

/* ----------------------------------------------------------------
 * Health check
 * ---------------------------------------------------------------- */

static void
handle_health(void)
{
    char fpath[1024], apath[1024];
    DIR *d;
    struct dirent *ent;
    int format_count = 0, atom_count = 0;
    char response[256];

    snprintf(fpath, sizeof(fpath), "%s/formats", data_dir);
    d = opendir(fpath);
    if (d) {
        while ((ent = readdir(d)) != NULL) {
            if (ent->d_name[0] != '.') format_count++;
        }
        closedir(d);
    }

    snprintf(apath, sizeof(apath), "%s/atoms/s", data_dir);
    d = opendir(apath);
    if (d) {
        while ((ent = readdir(d)) != NULL) {
            if (ent->d_name[0] != '.') atom_count++;
        }
        closedir(d);
    }

    snprintf(response, sizeof(response),
             "{\"status\":\"ok\",\"format_count\":%d,\"atom_count\":%d}",
             format_count, atom_count);
    json_ok(response);
}

/* ----------------------------------------------------------------
 * Main routing
 * ---------------------------------------------------------------- */

int
main(int argc, char **argv)
{
    const char *method = getenv("REQUEST_METHOD");
    const char *path_info = getenv("PATH_INFO");
    const char *env_data_dir = getenv("KORVO_DATA_DIR");
    int body_len = 0;
    char *body = NULL;

    if (env_data_dir) {
        data_dir = env_data_dir;
    }
    ensure_data_dirs();

    if (!method || !path_info) {
        json_error("{\"error\":\"not a CGI request\"}");
        return 0;
    }

    /* Read POST body if present */
    if (strcmp(method, "POST") == 0) {
        body = read_post_body(&body_len);
    }

    /* Route: /v1/health */
    if (strcmp(path_info, "/v1/health") == 0) {
        handle_health();
        free(body);
        return 0;
    }

    /* Route: /v1/formats */
    if (strcmp(path_info, "/v1/formats") == 0 && strcmp(method, "POST") == 0) {
        if (!body) {
            json_error("{\"error\":\"missing request body\"}");
            return 0;
        }
        handle_post_format(body);
        free(body);
        return 0;
    }

    if (strncmp(path_info, "/v1/formats/", 12) == 0 && strcmp(method, "GET") == 0) {
        const char *hex_id = path_info + 12;
        if (strlen(hex_id) == 0) {
            json_error("{\"error\":\"missing format id\"}");
        } else {
            handle_get_format(hex_id);
        }
        free(body);
        return 0;
    }

    /* Route: /v1/atoms */
    if (strcmp(path_info, "/v1/atoms") == 0 && strcmp(method, "POST") == 0) {
        if (!body) {
            json_error("{\"error\":\"missing request body\"}");
            return 0;
        }
        handle_post_atom(body);
        free(body);
        return 0;
    }

    if (strncmp(path_info, "/v1/atoms/by-string/", 20) == 0 && strcmp(method, "GET") == 0) {
        const char *str = path_info + 20;
        char decoded[512];
        url_safe_decode(str, decoded, sizeof(decoded));
        handle_get_atom_by_string(decoded);
        free(body);
        return 0;
    }

    if (strncmp(path_info, "/v1/atoms/by-value/", 19) == 0 && strcmp(method, "GET") == 0) {
        int atom = atoi(path_info + 19);
        handle_get_atom_by_value(atom);
        free(body);
        return 0;
    }

    free(body);
    respond(404, "Not Found", "application/json", "{\"error\":\"unknown endpoint\"}");
    return 0;
}
