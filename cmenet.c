/***** Includes *****/
#include "config.h"
#if defined (__INTEL_COMPILER)
#  pragma warning (disable: 1418)
#endif

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>

#include <enet/enet.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
 
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <atl.h>
#include "evpath.h"
#include "cm_transport.h"


typedef struct _queued_data {
    struct _queued_data *next;
    struct enet_connection_data *econn_d;
    ENetPacket *packet;
} *queued_data;

typedef struct func_list_item {
    select_list_func func;
    void *arg1;
    void *arg2;
} FunctionListElement;

typedef struct enet_client_data {
    CManager cm;
    char *hostname;
    int listen_port;
    CMtrans_services svc;
    ENetHost *server;
    queued_data pending_data;
    int wake_write_fd;
    int wake_read_fd;
    enet_uint32 last_host_service_zero_return;
} *enet_client_data_ptr;

typedef struct enet_connection_data {
    char *remote_host;
    int remote_IP;     /* in host byte order */
    int remote_contact_port;
    ENetPeer *peer;
    CMbuffer read_buffer;
    int read_buffer_len;
    ENetPacket *packet;
    enet_client_data_ptr ecd;
    CMConnection conn;
} *enet_conn_data_ptr;

static atom_t CM_PEER_IP = -1;
static atom_t CM_PEER_LISTEN_PORT = -1;
static atom_t CM_NETWORK_POSTFIX = -1;
static atom_t CM_ENET_PORT = -1;
static atom_t CM_ENET_HOSTNAME = -1;
static atom_t CM_ENET_ADDR = -1;
static atom_t CM_ENET_CONN_TIMEOUT = -1;
static atom_t CM_ENET_CONN_REUSE = -1;
static atom_t CM_TRANSPORT = -1;

static enet_uint32 enet_host_service_warn_interval = 0;

extern attr_list
libcmenet_LTX_non_blocking_listen(CManager cm, CMtrans_services svc,
				  transport_entry trans, attr_list listen_info);

static int
check_host(char *hostname, void *sin_addr)
{
    (void)hostname; (void)sin_addr;
#ifdef HAS_STRUCT_HOSTEN
    struct hostent *host_addr;
    host_addr = gethostbyname(hostname);
    if (host_addr == NULL) {
	struct in_addr addr;
	if (inet_aton(hostname, &addr) == 0) {
	    /* 
	     *  not translatable as a hostname or 
	     * as a dot-style string IP address
	     */
	    return 0;
	}
	assert(sizeof(int) == sizeof(struct in_addr));
	*((int *) sin_addr) = *((int*) &addr);
    } else {
	memcpy(sin_addr, host_addr->h_addr, host_addr->h_length);
    }
    return 1;
#endif
    printf("Check host called, unimplemented\n");
    return 0;
}

static enet_conn_data_ptr 
create_enet_conn_data(CMtrans_services svc)
{
    enet_conn_data_ptr enet_conn_data =
    svc->malloc_func(sizeof(struct enet_connection_data));
    enet_conn_data->remote_host = NULL;
    enet_conn_data->remote_contact_port = -1;
    enet_conn_data->read_buffer = NULL;
    enet_conn_data->read_buffer_len = 1;
    return enet_conn_data;
}

static void *
enet_accept_conn(enet_client_data_ptr ecd, transport_entry trans, 
		 ENetAddress *address);

static void free_func(void *packet)
{
    /* Clean up the packet now that we're done using it. */
    enet_packet_destroy ((ENetPacket*)packet);
}

static void
handle_packet(CManager cm, CMtrans_services svc, transport_entry trans, enet_conn_data_ptr econn_d, ENetPacket *packet)
{
    CMbuffer cb;
    svc->trace_out(cm, "A packet of length %u was received.\n",
                   (unsigned int) packet->dataLength);
    econn_d->read_buffer_len = packet->dataLength;
    cb = svc->create_data_and_link_buffer(cm, 
                                          packet->data, 
                                          econn_d->read_buffer_len);
    econn_d->read_buffer = cb;
    cb->return_callback = free_func;
    cb->return_callback_data = packet;
    econn_d->packet = packet;

    /* kick this upstairs */
    trans->data_available(trans, econn_d->conn);
    svc->return_data_buffer(trans->cm, cb);
}
	    
static
void
enet_service_network(CManager cm, void *void_trans)
{
    transport_entry trans = (transport_entry) void_trans;
    enet_client_data_ptr ecd = (enet_client_data_ptr) trans->trans_data;
    CMtrans_services svc = ecd->svc;
    ENetEvent event;
    
    if (!ecd->server) return;
    if (!(CM_LOCKED(svc, ecd->cm))) {
	printf("Enet service network, CManager not locked\n");
    }

    while (ecd->pending_data) {
        svc->trace_out(cm, "ENET Handling pending data\n");
        queued_data entry = ecd->pending_data;
        ecd->pending_data = entry->next;
        handle_packet(cm, svc, trans, entry->econn_d, entry->packet);
        free(entry);
    }

    while (ecd->server && (enet_host_service (ecd->server, & event, 0) > 0)) {
        if (enet_host_service_warn_interval && 
            (enet_time_get() > (ecd->last_host_service_zero_return + enet_host_service_warn_interval))) {
            fprintf(stderr, "WARNING, time between zero return for enet_host_service = %d msecs\n",
                    enet_time_get() - ecd->last_host_service_zero_return);
        }

        switch (event.type) {
	case ENET_EVENT_TYPE_NONE:
	    break;
        case ENET_EVENT_TYPE_CONNECT: {
	    void *enet_connection_data;
            struct in_addr addr;
            addr.s_addr = event.peer->address.host;
	    svc->trace_out(cm, "A new client connected from %s:%u.\n", 
			   inet_ntoa(addr),
			   event.peer->address.port);

	    enet_connection_data = enet_accept_conn(ecd, trans, &event.peer->address);

            /* Store any relevant client information here. */
            svc->trace_out(cm, "ENET ========   Assigning peer %p has data %p\n", event.peer, enet_connection_data);
            event.peer->data = enet_connection_data;
	    ((enet_conn_data_ptr)enet_connection_data)->peer = event.peer;

            break;
	}
        case ENET_EVENT_TYPE_RECEIVE: {
	    enet_conn_data_ptr econn_d = event.peer->data;
            if (econn_d) {
                handle_packet(cm, svc, trans, event.peer->data, event.packet);
            } else {
                struct in_addr addr;
                addr.s_addr = event.peer->address.host;
                svc->trace_out(cm, "ENET  ====== virgin peer, address is %s, port %u.\n", inet_ntoa(addr), event.peer->address.port);
                svc->trace_out(cm, "ENET  ====== DiSCARDING DATA\n");
            }
            break;
	}           
        case ENET_EVENT_TYPE_DISCONNECT: {
	    enet_conn_data_ptr enet_conn_data = event.peer->data;
	    svc->trace_out(cm, "Got a disconnect on connection %p\n",
		event.peer->data);

            enet_conn_data = event.peer->data;
	    enet_conn_data->read_buffer_len = -1;
            svc->connection_fail(enet_conn_data->conn);
        }
	}
    }
    ecd->last_host_service_zero_return = enet_time_get();
}

static
void
enet_service_network_lock(CManager cm, void *void_trans)
{
    transport_entry trans = (transport_entry) void_trans;
    enet_client_data_ptr ecd = (enet_client_data_ptr) trans->trans_data;
    CMtrans_services svc = ecd->svc;
    ACQUIRE_CM_LOCK(svc, cm);
    enet_service_network(cm, void_trans);
    DROP_CM_LOCK(svc, cm);
}

static
void
read_wake_fd_and_service(CManager cm, void *void_trans)
{
    transport_entry trans = (transport_entry) void_trans;
    enet_client_data_ptr ecd = (enet_client_data_ptr) trans->trans_data;

    char buffer;
    int fd = ecd->wake_read_fd;

#ifdef HAVE_WINDOWS_H
    recv(fd, &buffer, 1, 0);
#else
    if (read(fd, &buffer, 1) != 1) {
	perror("wake read failed\n");
    }
#endif

    enet_service_network(cm, void_trans);
}

#ifdef NOTDEF
static
void 
dump_sockaddr(who, sa)
char *who;
struct sockaddr_in *sa;
{
    unsigned char *addr;

    addr = (unsigned char *) &(sa->sin_addr.s_addr);

    printf("%s: family=%d port=%d addr=%d.%d.%d.%d\n",
	   who,
	   ntohs(sa->sin_family),
	   ntohs(sa->sin_port),
	   addr[0], addr[1], addr[2], addr[3]);
}

static
void 
dump_sockinfo(msg, fd)
char *msg;
int fd;
{
    int nl;
    struct sockaddr_in peer, me;

    printf("Dumping sockinfo for fd=%d: %s\n", fd, msg);

    nl = sizeof(me);
    getsockname(fd, (struct sockaddr *) &me, &nl);
    dump_sockaddr("Me", &me);

    nl = sizeof(peer);
    getpeername(fd, (struct sockaddr *) &peer, &nl);
    dump_sockaddr("Peer", &peer);
}

#endif

static int conn_reuse = 1;

/* 
 * Accept enet connection
 */
static void *
enet_accept_conn(enet_client_data_ptr ecd, transport_entry trans, 
		 ENetAddress *address)
{
    CMtrans_services svc = ecd->svc;
    enet_conn_data_ptr enet_conn_data;

    CMConnection conn;
    attr_list conn_attr_list = NULL;;

    enet_conn_data = create_enet_conn_data(svc);
    enet_conn_data->ecd = ecd;
    conn_attr_list = create_attr_list();
    conn = svc->connection_create(trans, enet_conn_data, conn_attr_list);
    enet_conn_data->conn = conn;

    add_int_attr(conn_attr_list, CM_PEER_IP, ntohl(address->host));
    enet_conn_data->remote_IP = ntohl(address->host);   /* remote_IP is in host byte order */
    if (!conn_reuse) {
        enet_conn_data->remote_contact_port = -1;
    } else {
        enet_conn_data->remote_contact_port = address->port;
    }

    if (enet_conn_data->remote_host != NULL) {
	svc->trace_out(trans->cm, "Accepted ENET RUDP connection from host \"%s\"",
		       enet_conn_data->remote_host);
    } else {
	svc->trace_out(trans->cm, "Accepted ENET RUDP connection from UNKNOWN host");
    }
    add_attr(conn_attr_list, CM_PEER_LISTEN_PORT, Attr_Int4,
	     (attr_value) (long)enet_conn_data->remote_contact_port);
    struct in_addr addr;
    addr.s_addr = htonl(enet_conn_data->remote_IP);
    svc->trace_out(trans->cm, "Remote host (IP %s) is listening at port %d\n",
		   inet_ntoa(addr),
		   enet_conn_data->remote_contact_port);
    free_attr_list(conn_attr_list);

    /* 
     * try flushing connection verify message here to make 
     * sure it's established 
     */
    enet_host_flush(ecd->server);

    return enet_conn_data;
}

extern void
libcmenet_LTX_shutdown_conn(CMtrans_services svc, enet_conn_data_ptr scd)
{
    svc->connection_deref(scd->conn);
    if (scd->remote_host) free(scd->remote_host);
    free(scd);
}


static int
initiate_conn(CManager cm, CMtrans_services svc, transport_entry trans,
	      attr_list attrs, enet_conn_data_ptr enet_conn_data,
	      attr_list conn_attr_list)
{
    int int_port_num;
    enet_client_data_ptr ecd = (enet_client_data_ptr) trans->trans_data;
    char *host_name;
    int host_ip = 0;
    struct in_addr sin_addr;
    (void)conn_attr_list;
    int timeout = 5000;   /* connection time out default 5 seconds */

    if (!query_attr(attrs, CM_ENET_HOSTNAME, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & host_name)) {
	svc->trace_out(cm, "CMEnet transport found no CM_ENET_HOSTNAME attribute");
	host_name = NULL;
    } else {
        svc->trace_out(cm, "CMEnet transport connect to host %s", host_name);
    }
    if (!query_attr(attrs, CM_ENET_ADDR, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & host_ip)) {
	svc->trace_out(cm, "CMEnet transport found no CM_ENET_ADDR attribute");
	/* wasn't there */
	host_ip = 0;
    } else {
        svc->trace_out(cm, "CMEnet transport connect to host_IP %lx", host_ip);
    }
    /* HOST_IP is in HOST BYTE ORDER */
    if ((host_name == NULL) && (host_ip == 0)) {
	printf("No host no IP\n");
	return 0;
    }

    if (!query_attr(attrs, CM_ENET_PORT, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & int_port_num)) {
	svc->trace_out(cm, "CMEnet transport found no CM_ENET_PORT attribute");
	return 0;
    } else {
        svc->trace_out(cm, "CMEnet transport connect to port %d", int_port_num);
    }

    if (!query_attr(attrs, CM_ENET_CONN_TIMEOUT, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & timeout)) {
	svc->trace_out(cm, "CMEnet transport found no CM_ENET_CONN_TIMEOUT attribute");
    } else {
        svc->trace_out(cm, "CMEnet transport connection timeout set to %d msecs", timeout);
    }
    if (!query_attr(attrs, CM_ENET_CONN_REUSE, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & conn_reuse)) {
	svc->trace_out(cm, "CMEnet transport found no CM_ENET_CONN_REUSE attribute");
    } else {
        svc->trace_out(cm, "CMEnet transport connection reuse set to %d", conn_reuse);
    }

    /* ENET connection, host_name is the machine name */
    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;
    sin_addr.s_addr = htonl(host_ip);

    if (host_name) {
	enet_address_set_host (& address, host_name);
	sin_addr.s_addr = address.host;
	svc->trace_out(cm, "Attempting ENET RUDP connection, USING host=\"%s\", IP = %s, port %d",
		       host_name == 0 ? "(unknown)" : host_name, 
		       inet_ntoa(sin_addr),
		       int_port_num);
    } else {
	address.host = ntohl(host_ip);
	svc->trace_out(cm, "Attempting ENET RUDP connection, USING IP = %s, port %d",
		       inet_ntoa(sin_addr),
		       int_port_num);
    }
    address.port = (unsigned short) int_port_num;

    if (ecd->server == NULL) {
	attr_list l = libcmenet_LTX_non_blocking_listen(cm, svc, trans, NULL);
	if (l) free_attr_list(l);
    }

    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect (ecd->server, & address, 1, 0);    
    peer->data = enet_conn_data;
    svc->trace_out(cm, "ENET ========   On init Assigning peer %p has data %p\n", peer, enet_conn_data);
    if (peer == NULL)
    {
       fprintf (stderr, 
                "No available peers for initiating an ENet connection.\n");
       exit (EXIT_FAILURE);
    }
    
    /* Wait up to 'timeout' milliseconds for the connection attempt to succeed. */
    int finished = 0;
    int got_connection = 0;
    enet_uint32 end = enet_time_get() + timeout;
    while (!finished) {
        int ret = enet_host_service (ecd->server, & event, 100); 
        enet_uint32 now = enet_time_get();
        if (enet_host_service_warn_interval && 
            (enet_time_get() > (ecd->last_host_service_zero_return + enet_host_service_warn_interval))) {
            fprintf(stderr, "WARNING, time between zero return for enet_host_service = %d msecs\n",
                    enet_time_get() - ecd->last_host_service_zero_return);
        }
        if (now > end) {
            finished = 1;
        }
        if (ret <= 0) {
            ecd->last_host_service_zero_return = enet_time_get();
            continue;
        }
        switch(event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            if (event.peer != peer) {
                enet_conn_data_ptr enet_connection_data;
                struct in_addr addr;
                addr.s_addr = event.peer->address.host;
                svc->trace_out(cm, "A new client connected from %s:%u.\n", 
                               inet_ntoa(addr),
                               event.peer->address.port);
                
                enet_connection_data = enet_accept_conn(ecd, trans, &event.peer->address);
                
                /* Store any relevant client information here. */
                svc->trace_out(cm, "ENET ========   Assigning peer %p has data %p\n", event.peer, enet_connection_data);
                event.peer->data = enet_connection_data;
                ((enet_conn_data_ptr)enet_connection_data)->peer = event.peer;
                enet_host_flush (ecd->server);
            } else {
                enet_host_flush (ecd->server);
                svc->trace_out(cm, "Connection to %s:%d succeeded.\n", inet_ntoa(sin_addr), address.port);
                finished = 1;
                got_connection = 1;
            }
            break;
        }
        case ENET_EVENT_TYPE_NONE:
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            if (event.peer == peer) {
                enet_peer_reset (peer);
                
                svc->trace_out(cm, "Connection to %s:%d failed   type was %d.\n", inet_ntoa(sin_addr), address.port, event.type);
                return 0;
            } else {
                enet_conn_data_ptr enet_conn_data = event.peer->data;
                svc->trace_out(cm, "Got a disconnect on connection %p\n",
                               event.peer->data);
                
                enet_conn_data = event.peer->data;
                enet_conn_data->read_buffer_len = -1;
                svc->connection_fail(enet_conn_data->conn);
            }
            break;
        case ENET_EVENT_TYPE_RECEIVE: {
	    enet_conn_data_ptr econn_d = event.peer->data;
            queued_data entry = malloc(sizeof(*entry));
            entry->next = NULL;
            entry->econn_d = econn_d;
            entry->packet = event.packet;
            /* add at the end */
            if (econn_d->ecd->pending_data == NULL) {
                econn_d->ecd->pending_data = entry;
            } else {
                queued_data last = econn_d->ecd->pending_data;
                while (last->next != NULL) {
                    last = last->next;
                }
                last->next = entry;
            }
            break;
        }
        }
    }

    if (!got_connection) {
        svc->trace_out(cm, "--> Connection failed because of timeout");
        return 0;
    }
    svc->trace_out(cm, "--> Connection established\n");
    enet_conn_data->remote_host = host_name == NULL ? NULL : strdup(host_name);
    enet_conn_data->remote_IP = htonl(host_ip);
    enet_conn_data->remote_contact_port = int_port_num;
    enet_conn_data->ecd = ecd;
    enet_conn_data->peer = peer;
    peer->data = enet_conn_data;
    return 1;
}

/* 
 * Initiate a ENET RUDP connection with another CM.
 */
extern CMConnection
libcmenet_LTX_initiate_conn(CManager cm, CMtrans_services svc,
			    transport_entry trans, attr_list attrs)
{
    enet_conn_data_ptr enet_conn_data = create_enet_conn_data(svc);
    attr_list conn_attr_list = create_attr_list();
    CMConnection conn;

    if (!initiate_conn(cm, svc, trans, attrs, enet_conn_data, conn_attr_list))
	return NULL;

    add_attr(conn_attr_list, CM_PEER_LISTEN_PORT, Attr_Int4,
	     (attr_value) (long)enet_conn_data->remote_contact_port);
    conn = svc->connection_create(trans, enet_conn_data, conn_attr_list);
    enet_conn_data->conn = conn;
    free_attr_list(conn_attr_list);
    svc->connection_addref(conn);  /* one ref count went to CM, 
				the other to the user */

    return conn;
}


/* 
 * Check to see that if we were to attempt to initiate a connection as
 * indicated by the attribute list, would we be connecting to ourselves?
 * For enet, this involves checking to see if the host name is the 
 * same as ours and if the CM_ENET_PORT matches the one we are listening on.
 */
extern int
libcmenet_LTX_self_check(CManager cm, CMtrans_services svc, 
			 transport_entry trans, attr_list attrs)
{

    enet_client_data_ptr ecd = trans->trans_data;
    int host_addr;
    int int_port_num;
    char *host_name;
    char my_host_name[256];
    static int IP = 0;   /* always in host byte order */

    get_IP_config(my_host_name, sizeof(host_name), &IP, NULL, NULL, NULL,
		  NULL, svc->trace_out, (void *)cm);

    if (IP == 0) {
	IP = ntohl(INADDR_LOOPBACK);
    }
    if (!query_attr(attrs, CM_ENET_HOSTNAME, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & host_name)) {
	svc->trace_out(cm, "CMself check CMEnet transport found no CM_ENET_HOSTNAME attribute");
	host_name = NULL;
    }
    if (!query_attr(attrs, CM_ENET_ADDR, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & host_addr)) {
	svc->trace_out(cm, "CMself check CMEnet transport found no CM_ENET_ADDR attribute");
	if (host_name == NULL) return 0;
	host_addr = 0;
    }
    if (!query_attr(attrs, CM_ENET_PORT, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & int_port_num)) {
	svc->trace_out(cm, "CMself check CMEnet transport found no CM_ENET_PORT attribute");
	return 0;
    }
    //get_qual_hostname(my_host_name, sizeof(my_host_name), svc, NULL, NULL);

    if (host_name && (strcmp(host_name, my_host_name) != 0)) {
	svc->trace_out(cm, "CMself check - Hostnames don't match");
	return 0;
    }
    if (host_addr && (IP != host_addr)) {
	svc->trace_out(cm, "CMself check - Host IP addrs don't match, %lx, %lx", IP, host_addr);
	return 0;
    }
    if (int_port_num != ecd->listen_port) {
	svc->trace_out(cm, "CMself check - Ports don't match, %d, %d", int_port_num, ecd->listen_port);
	return 0;
    }
    svc->trace_out(cm, "CMself check returning TRUE");
    return 1;
}

extern int
libcmenet_LTX_connection_eq(CManager cm, CMtrans_services svc,
			    transport_entry trans, attr_list attrs,
			    enet_conn_data_ptr ecd)
{

    int int_port_num;
    int requested_IP = -1;
    char *host_name = NULL;

    (void) trans;
    if (!query_attr(attrs, CM_ENET_HOSTNAME, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & host_name)) {
	svc->trace_out(cm, "CMEnet transport found no CM_ENET_HOST attribute");
    }
    if (!query_attr(attrs, CM_ENET_PORT, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & int_port_num)) {
	svc->trace_out(cm, "Conn Eq CMenet transport found no CM_ENET_PORT attribute");
	return 0;
    }
    if (!query_attr(attrs, CM_ENET_ADDR, /* type pointer */ NULL,
    /* value pointer */ (attr_value *)(long) & requested_IP)) {
	svc->trace_out(cm, "CMENET transport found no CM_ENET_ADDR attribute");
    }
    if (requested_IP == -1) {
	check_host(host_name, (void *) &requested_IP);
	requested_IP = ntohl(requested_IP);
        struct in_addr addr;
        addr.s_addr = htonl(requested_IP);
	svc->trace_out(cm, "IP translation for hostname %s is %s", host_name,
		       inet_ntoa(addr));
    }
    /* requested IP is in host byte order */
    if (ecd->peer->state != ENET_PEER_STATE_CONNECTED) {
        svc->trace_out(cm, "ENET Conn_eq returning FALSE, peer not connected");
        return 0;
    }
    struct in_addr addr1, addr2;
    addr1.s_addr = htonl(ecd->remote_IP);
    addr2.s_addr = htonl(requested_IP);
    svc->trace_out(cm, "ENET Conn_eq comparing IP/ports %s/%d and %s/%d",
		   inet_ntoa(addr1), ecd->remote_contact_port,
                   inet_ntoa(addr2), int_port_num);
    if ((ecd->remote_IP == requested_IP) &&    /* both in host byte order */
	(ecd->remote_contact_port == int_port_num)) {
	svc->trace_out(cm, "ENET Conn_eq returning TRUE");
	return 1;
    }
    svc->trace_out(cm, "ENET Conn_eq returning FALSE");
    return 0;
}

static attr_list
build_listen_attrs(CManager cm, CMtrans_services svc, enet_client_data_ptr ecd,
		   attr_list listen_info, int int_port_num)
{
    char host_name[256];
    attr_list ret_list;
    int IP;
    int use_hostname = 0;
    
    svc->trace_out(cm, "CMEnet listen succeeded on port %d",
		       int_port_num);
    get_IP_config(host_name, sizeof(host_name), &IP, NULL, NULL,
		  &use_hostname, listen_info, svc->trace_out, (void *)cm);

    ret_list = create_attr_list();

    if (ecd) {
	ecd->hostname = strdup(host_name);
	ecd->listen_port = int_port_num;
    }
    if ((IP != 0) && !use_hostname) {
	add_attr(ret_list, CM_ENET_ADDR, Attr_Int4,
		 (attr_value) (long)IP);
    }
    if ((getenv("CMEnetsUseHostname") != NULL) || 
	use_hostname) {
	add_attr(ret_list, CM_ENET_HOSTNAME, Attr_String,
		 (attr_value) strdup(host_name));
    } else if (IP == 0) {
        add_int_attr(ret_list, CM_ENET_ADDR, INADDR_LOOPBACK);
    }
    add_attr(ret_list, CM_ENET_PORT, Attr_Int4,
	     (attr_value) (long)int_port_num);
    
    add_attr(ret_list, CM_TRANSPORT, Attr_String,
	     (attr_value) strdup("enet"));
    return ret_list;
}

static void
wake_enet_server_thread(enet_client_data_ptr enet_data)
{
    static char buffer = 'W';  /* doesn't matter what we write */
    if (enet_data->wake_write_fd != -1) {
#ifdef HAVE_WINDOWS_H
	send(enet_data->wake_write_fd, &buffer, 1, 0);
#else
	if (write(enet_data->wake_write_fd, &buffer, 1) != 1) {
	    printf("Whoops, wake write failed\n");
	}
#endif
    }
}

/* 
 * Create an IP socket for connection from other CMs
 */
extern attr_list
libcmenet_LTX_non_blocking_listen(CManager cm, CMtrans_services svc,
				  transport_entry trans, attr_list listen_info)
{
    enet_client_data_ptr enet_data = trans->trans_data;
    ENetAddress address;
    ENetHost * server;


    int attr_port_num = 0;
    u_short port_num = 0;

    /* 
     *  Check to see if a bind to a specific port was requested
     */
    if (listen_info != NULL
	&& !query_attr(listen_info, CM_ENET_PORT,
		       NULL, (attr_value *)(long) & attr_port_num)) {
	port_num = 0;
    } else {
	if (attr_port_num > USHRT_MAX || attr_port_num < 0) {
	    fprintf(stderr, "Requested port number %d is invalid\n", attr_port_num);
	    return NULL;
	}
	port_num = attr_port_num;
    }

    svc->trace_out(cm, "CMEnet begin listen, requested port %d", attr_port_num);

    address.host = ENET_HOST_ANY;

    if (enet_data->server != NULL) {
	/* we're already listening */
        if (port_num == 0) {
	    /* not requesting a specific port, return what we have */
	    return build_listen_attrs(cm, svc, NULL, listen_info, enet_data->listen_port);
	} else {
	    printf("CMlisten_specific() requesting a specific port follows other Enet operation which initiated listen at another port.  Only one listen allowed, second listen fails.\n");
	    return NULL;
	}
    }
    if (port_num != 0) {
	/* Bind the server to the default localhost.     */
	/* A specific host address can be specified by   */
	/* enet_address_set_host (& address, "x.x.x.x"); */

	address.port = port_num;

	svc->trace_out(cm, "CMEnet trying to bind selected port %d", port_num);
	server = enet_host_create (& address /* the address to bind the server host to */, 
				   0      /* allow up to 4095 clients and/or outgoing connections */,
				   1      /* allow up to 2 channels to be used, 0 and 1 */,
				   0      /* assume any amount of incoming bandwidth */,
				   0      /* assume any amount of outgoing bandwidth */);
	if (server == NULL) {
	    fprintf (stderr, 
		     "An error occurred while trying to create an ENet server host.\n");
	    return NULL;
	}
	enet_data->server = server;
    } else {
	long seedval = time(NULL) + getpid();
	/* port num is free.  Constrain to range 26000 : 26100 */
	int low_bound, high_bound;
	int size;
	int tries;
	srand48(seedval);
	get_IP_config(NULL, 0, NULL, &low_bound, &high_bound,
		      NULL, listen_info, svc->trace_out, (void *)cm);

    restart:
	size = high_bound - low_bound;
	tries = 10;
	while (tries > 0) {
	    int target = low_bound + size * drand48();
	    address.port = target;
	    svc->trace_out(cm, "CMEnet trying to bind port %d", target);

	    server = enet_host_create (& address /* the address to bind the server host to */, 
				       0     /* 0 means dynamic alloc clients and/or outgoing connnections */,
				       1      /* allow up to 2 channels to be used, 0 and 1 */,
				       0      /* assume any amount of incoming bandwidth */,
				       0      /* assume any amount of outgoing bandwidth */);
	    tries--;
	    if (server != NULL) tries = 0;
	    if (tries == 5) {
		/* try reseeding in case we're in sync with another process */
		srand48(time(NULL) + getpid());
	    }
	}
	if (server == NULL) {
	    high_bound += 100;
	    goto restart;
	}
	enet_data->server = server;
    }
    svc->fd_add_select(cm, enet_host_get_sock_fd (server), 
		       (select_list_func) enet_service_network, (void*)cm, (void*)trans);

    svc->add_periodic_task(cm, 0, 100, (CMPollFunc) enet_service_network_lock, (void*)trans);

    svc->trace_out(enet_data->cm, "CMENET Adding read_wake_fd as action on fd %d",
		   enet_data->wake_read_fd);

    svc->fd_add_select(cm, enet_data->wake_read_fd, (select_list_func)read_wake_fd_and_service, 
                       (void*)cm, (void*)trans);

    return build_listen_attrs(cm, svc, enet_data, listen_info, address.port);
}

#if defined(HAVE_WINDOWS_H) && !defined(NEED_IOVEC_DEFINE)
#define NEED_IOVEC_DEFINE
#endif

#ifdef NEED_IOVEC_DEFINE
struct iovec {
    void *iov_base;
    int iov_len;
};

#endif

extern void *
libcmenet_LTX_read_block_func(CMtrans_services svc,
			      enet_conn_data_ptr conn_data, int *actual_len,
			      int *offset_ptr)
{
    CMbuffer cb;

    if (conn_data->read_buffer_len == -1) return NULL;

    *actual_len = conn_data->read_buffer_len;
    *offset_ptr = 0;
    cb = conn_data->read_buffer;
    conn_data->read_buffer_len = 0;
    conn_data->read_buffer = NULL;
    return cb;
}

#ifdef CURRENT_UTC_TIME_NEEDED
static
void current_utc_time(struct timespec *ts)
{
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_REALTIME, ts);
#endif
}
#endif

#ifdef TIME_DIFF_USED
static struct timespec time_diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
	temp.tv_sec = end.tv_sec-start.tv_sec-1;
	temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
	temp.tv_sec = end.tv_sec-start.tv_sec;
	temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}
#endif

extern int
libcmenet_LTX_writev_func(CMtrans_services svc, enet_conn_data_ptr ecd,
			  struct iovec *iov, int iovcnt, attr_list attrs)
{
    int i;
    int length = 0;
    static time_t last_flush_call = 0;

    (void) attrs;
    for (i = 0; i < iovcnt; i++) {
	length += iov[i].iov_len;
    }

    svc->trace_out(ecd->ecd->cm, "CMENET vector write of %d bytes on peer %p",
		   length, ecd->peer);

   /* Create a reliable packet of the right size */
    ENetPacket * packet = enet_packet_create (NULL, length, 
					      ENET_PACKET_FLAG_RELIABLE);

    length = 0;
    /* copy in the data */
    for (i = 0; i < iovcnt; i++) {
	memcpy(packet->data + length, iov[i].iov_base, iov[i].iov_len);
	length += iov[i].iov_len;
    }

    /* Send the packet to the peer over channel id 0. */
    if (enet_peer_send (ecd->peer, 0, packet) == -1) {
        enet_packet_destroy(packet);
        svc->trace_out(ecd->ecd->cm, "ENET  ======  failed to send a packet to peer %p, state %d\n", ecd->peer, ecd->peer->state);
	return -1;
    }

    wake_enet_server_thread(ecd->ecd);

    if (last_flush_call == 0) {
	enet_host_flush(ecd->ecd->server);
	last_flush_call = time(NULL);
    } else {
	time_t now = time(NULL);
	if (now > last_flush_call) {
	    last_flush_call = now;
	    enet_host_flush(ecd->ecd->server);
	}
    }
    return iovcnt;
}


static int enet_global_init = 0;

static void
free_enet_data(CManager cm, void *ecdv)
{
    enet_client_data_ptr ecd = (enet_client_data_ptr) ecdv;
    CMtrans_services svc = ecd->svc;
    (void)cm;
    if (ecd->hostname != NULL)
	svc->free_func(ecd->hostname);
    svc->free_func(ecd);
}

static void
shutdown_enet_thread
(CManager cm, void *ecdv)
{
    enet_client_data_ptr ecd = (enet_client_data_ptr) ecdv;
    CMtrans_services svc = ecd->svc;
    (void)cm;
    if (ecd->server != NULL) {
	ENetHost * server = ecd->server;
	enet_host_flush(ecd->server);
	svc->fd_remove_select(cm, enet_host_get_sock_fd (server));
	ecd->server = NULL;
	enet_host_destroy(server);
    }
}

extern void *
libcmenet_LTX_initialize(CManager cm, CMtrans_services svc,
			 transport_entry trans, attr_list attrs)
{
    static int atom_init = 0;
    int filedes[2];
    char *env = getenv("ENET_HOST_SERVICE_WARN_INTERVAL");

    enet_client_data_ptr enet_data;
    (void)attrs;
    svc->trace_out(cm, "Initialize ENET reliable UDP transport built in %s",
		   EVPATH_LIBRARY_BUILD_DIR);
    if (enet_global_init == 0) {
	if (enet_initialize () != 0) {
	    fprintf (stderr, "An error occurred while initializing ENet.\n");
	    //return EXIT_FAILURE;
	}
        enet_time_set(0);   /* rollover in 50 days */
    }
    if (atom_init == 0) {
	CM_ENET_HOSTNAME = attr_atom_from_string("CM_ENET_HOST");
	CM_ENET_PORT = attr_atom_from_string("CM_ENET_PORT");
	CM_ENET_ADDR = attr_atom_from_string("CM_ENET_ADDR");
	CM_TRANSPORT = attr_atom_from_string("CM_TRANSPORT");
	CM_PEER_IP = attr_atom_from_string("PEER_IP");
	CM_PEER_LISTEN_PORT = attr_atom_from_string("PEER_LISTEN_PORT");
	CM_NETWORK_POSTFIX = attr_atom_from_string("CM_NETWORK_POSTFIX");
	CM_ENET_CONN_TIMEOUT = attr_atom_from_string("CM_ENET_CONN_TIMEOUT");
	CM_ENET_CONN_REUSE = attr_atom_from_string("CM_ENET_CONN_REUSE");
	atom_init++;
    }
    if (env) {
        sscanf(env, "%d", &enet_host_service_warn_interval);
        fprintf(stderr, "DEBUG: Setting enet_host_service_warn_interval to %d\n", enet_host_service_warn_interval);
    }
    enet_data = svc->malloc_func(sizeof(struct enet_client_data));
    enet_data->cm = cm;
    enet_data->hostname = NULL;
    enet_data->listen_port = -1;
    enet_data->svc = svc;
    enet_data->server = NULL;
    enet_data->pending_data = NULL;

    if (pipe(filedes) != 0) {
	perror("Pipe for wake not created.  ENET wake mechanism inoperative.");
	return NULL;
    }
    enet_data->wake_read_fd = filedes[0];
    enet_data->wake_write_fd = filedes[1];
    svc->add_shutdown_task(cm, shutdown_enet_thread, (void *) enet_data, SHUTDOWN_TASK);
    svc->add_shutdown_task(cm, free_enet_data, (void *) enet_data, FREE_TASK);
    return (void *) enet_data;
}

extern transport_entry
cmenet_add_static_transport(CManager cm, CMtrans_services svc)
{
    transport_entry transport;
    transport = svc->malloc_func(sizeof(struct _transport_item));
    memset(transport, 0, sizeof(*transport));
    transport->trans_name = strdup("enet");
    transport->cm = cm;
    transport->transport_init = (CMTransport_func)libcmenet_LTX_initialize;
    transport->listen = (CMTransport_listen_func)libcmenet_LTX_non_blocking_listen;
    transport->initiate_conn = (CMConnection(*)())libcmenet_LTX_initiate_conn;
    transport->self_check = (int(*)())libcmenet_LTX_self_check;
    transport->connection_eq = (int(*)())libcmenet_LTX_connection_eq;
    transport->shutdown_conn = (CMTransport_shutdown_conn_func)libcmenet_LTX_shutdown_conn;
    transport->read_block_func = (CMTransport_read_block_func)libcmenet_LTX_read_block_func;
    transport->read_to_buffer_func = (CMTransport_read_to_buffer_func)NULL;
    transport->writev_func = (CMTransport_writev_func)libcmenet_LTX_writev_func;
    transport->get_transport_characteristics = NULL;
    if (transport->transport_init) {
	transport->trans_data = transport->transport_init(cm, svc, transport);
    }
    return transport;
}
