/*
  SST connection tool.


Run without parameters:  Listen mode

Say what listen parameters are in use (get from SST, directly from listen
output). Write connection file.  Wait for file open.


Run with -c parameter
Look for connection info. say what connection parameters are read and being
tried.  Say if connection succeeds, fails or times out.


SST info mode:
What interfaces are available.   What IP addresses are associated with each.  if
a hostname is identified using gethostname and getdomainname and it translates
to any interfaces.
*/

#include <arpa/inet.h>
#include <assert.h>
#include <atl.h>
#include <ctype.h>
#include <float.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#include "adios2/ADIOSConfig.h"
#include <atl.h>
#include <evpath.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#include "sstmpidummy.h"
#endif
#include <pthread.h>

#include "sst.h"

#include "cp_internal.h"

static atom_t TRANSPORT = -1;
static atom_t IP_PORT = -1;
static atom_t IP_HOSTNAME = -1;
static atom_t IP_ADDR = -1;
static atom_t ENET_PORT = -1;
static atom_t ENET_HOSTNAME = -1;
static atom_t ENET_ADDR = -1;

struct option options[] = {
    {"help", no_argument, NULL, 'h'},   {"verbose", no_argument, NULL, 'v'},
    {"listen", no_argument, NULL, 'l'}, {"connect", no_argument, NULL, 'c'},
    {"info", no_argument, NULL, 'i'},   {NULL, 0, NULL, 0}};

static const char *optstring = "-hvci";

void displayHelp()
{
    fprintf(stderr, "Usage:  sst_conn_tool { -l | -c | -i }\n");
    fprintf(stderr, "  -l,-listen  Display connection parameters and wait for "
                    "an SST connection\n");
    fprintf(stderr, "  -c,-connect Attempt a connection to an already-running "
                    "instance of sst_conn_tool\n");
    fprintf(stderr,
            "  -i,-info    Display networking information on this host\n");
    fprintf(stderr, "  -h,-help    Display this message\n\n");
}

static void do_listen();
static void do_connect();

static void init_atoms()
{
    TRANSPORT = attr_atom_from_string("CM_TRANSPORT");
    IP_HOSTNAME = attr_atom_from_string("IP_HOST");
    IP_PORT = attr_atom_from_string("IP_PORT");
    IP_ADDR = attr_atom_from_string("IP_ADDR");
    ENET_HOSTNAME = attr_atom_from_string("CM_ENET_HOST");
    ENET_PORT = attr_atom_from_string("CM_ENET_PORT");
    ENET_ADDR = attr_atom_from_string("CM_ENET_ADDR");
}

int main(int argc, char **argv)
{
    int c;
    int verbose = 0, connect = 0, info = 0, listen = 0;
    while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1)
    {
        switch (c)
        {
        case 'h':
            displayHelp();
            return 1;
        case 'v':
            verbose = 1;
            break;
        case 'c':
            connect = 1;
            break;
        case 'i':
            info = 1;
            break;
        default:
            displayHelp();
            return 1;
        }
    }
    if (verbose + connect + info > 1)
    {
        fprintf(stderr,
                "Only one of -listen, -connect or -info can be specified\n\n");
        displayHelp();
        return 1;
    }
    MPI_Init(&argc, &argv);
    init_atoms();
    if (connect)
    {
        do_connect();
    }
    else if (info)
    {
    }
    else
    {
        do_listen();
    }
}

static void DecodeAttrList(const char *attrs, char **in_transport, char **in_ip,
                           char **in_hostname, int *in_port)
{
    attr_list listen_info = attr_list_from_string(attrs);
    char *transport = NULL;
    get_string_attr(listen_info, TRANSPORT, &transport);
    if (transport == NULL)
    {
        /* must be sockets */
        struct in_addr addr;
        int ip = -1, port = -1;
        get_int_attr(listen_info, IP_PORT, &port);
        get_int_attr(listen_info, IP_ADDR, &ip);
        addr.s_addr = htonl(ip);

        if (in_ip)
            *in_ip = strdup(inet_ntoa(addr));
        if (in_port)
            *in_port = port;
    }
    else if (strcmp(transport, "enet") == 0)
    {
        /* reliable UDP transport "enet" */
        struct in_addr addr;
        int ip = -1, port = -1;
        get_int_attr(listen_info, ENET_PORT, &port);
        get_int_attr(listen_info, ENET_ADDR, &ip);
        addr.s_addr = htonl(ip);
        if (in_ip)
            *in_ip = strdup(inet_ntoa(addr));
        if (in_port)
            *in_port = port;
    }
    else
    {
        dump_attr_list(listen_info);
    }
    if (in_transport && transport)
        *in_transport = strdup(transport);
}

static void ConnToolCallback(int dataID, const char *attrs, const char *data)
{
    char *IP = NULL, *transport = NULL, *hostname = NULL;
    int port = -1;
    DecodeAttrList(attrs, &transport, &IP, &hostname, &port);
    if (dataID == 0)
    {
        /* writer-side, prior to connection, giving info on listener network
         * parameters */
        if (!transport)
        {
            printf("\n\tSst writer is listening for TCP/IP connection at IP "
                   "%s, port %d\n\n",
                   IP, port);
        }
        else if (strcmp(transport, "enet") == 0)
        {
            printf("\n\tSst writer is listening for UDP connection at IP %s, "
                   "port %d\n\n",
                   IP, port);
        }
        else
        {
            printf(
                "\n\tWarning, unknown control network transport operating\n");
        }
        printf("\tSst connection tool waiting for connection...\n\n");
    }
    else if (dataID == 1)
    {
        /* reader-side, prior to connection, giving info on listener network
         * parameters */
        if (!transport)
        {
            printf("\n\tSst reader at IP %s, listen TCP/IP port %d\n\n", IP,
                   port);
        }
        else if (strcmp(transport, "enet") == 0)
        {
            printf("\n\tSst reader at IP %s, listening UDP port %d\n\n", IP,
                   port);
        }
        else
        {
            printf(
                "\n\tWarning, unknown control network transport operating\n");
        }
    }
    else if (dataID == 2)
    {
        if (!transport)
        {
            printf("\n\tAttempting TCP/IP connection to writer at IP %s, port "
                   "%d\n\n",
                   IP, port);
        }
        else if (strcmp(transport, "enet") == 0)
        {
            printf("\n\tAttempting UDP connection to writer at IP %s,a port "
                   "%d\n\n",
                   IP, port);
        }
        else
        {
            printf(
                "\n\tWarning, unknown control network transport operating\n");
        }
    }
}

static void do_connect()
{
    struct _SstParams Params;
    SstStream reader;
    memset(&Params, 0, sizeof(Params));
    SSTSetNetworkCallback(ConnToolCallback);
    Params.RendezvousReaderCount = 1;
    Params.ControlTransport = "enet";
    reader = SstReaderOpen("SstConnToolTemp", &Params, MPI_COMM_WORLD);
    if (reader)
    {
        printf("Connection success, all is well!\n");
        SstReaderClose(reader);
    }
    else
    {
        printf("This connection did not succeed.  Determine if IP addresses in "
               "use are appropriate, or if firewalls or other network-level "
               "artifacts might be causing a problem.\n\n");
    }
}

static void do_listen()
{
    struct _SstParams Params;
    SstStream writer;
    memset(&Params, 0, sizeof(Params));
    SSTSetNetworkCallback(ConnToolCallback);
    Params.RendezvousReaderCount = 1;
    //    Params.ControlTransport = "enet";
    writer = SstWriterOpen("SstConnToolTemp", &Params, MPI_COMM_WORLD);
    printf("Connection success!\n");
    SstWriterClose(writer);
}
