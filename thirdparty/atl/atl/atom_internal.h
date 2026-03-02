typedef struct _send_get_atom_msg {
    char *atom_string;
    atom_t atom;
} send_get_atom_msg, *send_get_atom_msg_ptr;


#define UDP_PORT 4444
#define TCP_PORT 4445

/* HTTP atom client (http_atom_client.c) */
extern char *atl_http_server_url;
extern int http_set_string_and_atom(const char *str, atom_t atom);
extern atom_t http_atom_from_string(const char *str);
extern char *http_string_from_atom(atom_t atom);

