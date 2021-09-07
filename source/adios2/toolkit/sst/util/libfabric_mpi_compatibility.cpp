/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test if the libfabric library is compatible with MPI
 *
 *  Created on: Sept 7, 2021
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#include <iostream>
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/adiosCommMPI.h"
#include <atl.h>
#include <evpath.h>
#include <pthread.h>

#include "sst.h"

#include "cp_internal.h"

MPI_Comm comm;    // Communicator of producers OR consumers
int mpi_rank;     // rank of process among producers OR consumers
int mpi_size;     // number of processes of producers OR consumers
int wrank, wsize; // rank and size in world comm
int nProducers;
int nConsumers;
bool amProducer;
adios2::helper::Comm ADIOSComm;

std::vector<int> allranks; // array for MPI_Gather()

void init_atoms();
void do_listen();
void do_connect();

void PrintUsage() noexcept
{
    std::cout << "Usage: libfabric_mpi_compatibility [producerRanks] "
              << std::endl;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    if (argc > 1)
    {
        amProducer = false;
        nProducers = 0;
        int j = 1;
        char *end;
        while (argc > j)
        {
            errno = 0;
            unsigned long v = std::strtoul(argv[j], &end, 10);
            if ((errno || (end != 0 && *end != '\0')) && !wrank)
            {
                std::string errmsg(
                    "ERROR: Invalid integer number in argument " +
                    std::to_string(j) + ": '" + std::string(argv[j]) + "'\n");
                PrintUsage();
                throw std::invalid_argument(errmsg);
            }
            if (v >= (unsigned long)wsize && !wrank)
            {
                std::string errmsg(
                    "ERROR: Argument " + std::to_string(j) + ": '" +
                    std::string(argv[j]) +
                    "' is larger than the total number of processes\n");
                PrintUsage();
                throw std::invalid_argument(errmsg);
            }
            if (v == (unsigned long)wrank)
            {
                amProducer = true;
                ++nProducers;
            }
            ++j;
        }
    }
    else
    {
        amProducer = (wrank < wsize / 2);
        nProducers = wsize / 2;
    }
    nConsumers = wsize - nProducers;
    std::cout << "Rank " << wrank << " is a "
              << (amProducer ? "Producer" : "Consumer") << std::endl;

    MPI_Comm_split(MPI_COMM_WORLD, (int)amProducer, 0, &comm);
    MPI_Comm_rank(comm, &mpi_rank);
    MPI_Comm_size(comm, &mpi_size);
    ADIOSComm = adios2::helper::CommWithMPI(comm);
    MPI_Barrier(comm);

    if (!wrank)
    {
        allranks.resize(wsize);
    }

    init_atoms();
    if (amProducer)
    {
        do_listen();
    }
    else
    {
        do_connect();
    }

    MPI_Finalize();
    return 0;
}

static atom_t TRANSPORT = -1;
static atom_t IP_PORT = -1;
/* static atom_t IP_HOSTNAME = -1; */
static atom_t IP_ADDR = -1;
static atom_t ENET_PORT = -1;
/* static atom_t ENET_HOSTNAME = -1; */
static atom_t ENET_ADDR = -1;

void init_atoms()
{
    TRANSPORT = attr_atom_from_string("CM_TRANSPORT");
    /* IP_HOSTNAME = attr_atom_from_string("IP_HOST"); */
    IP_PORT = attr_atom_from_string("IP_PORT");
    IP_ADDR = attr_atom_from_string("IP_ADDR");
    /* ENET_HOSTNAME = attr_atom_from_string("CM_ENET_HOST"); */
    ENET_PORT = attr_atom_from_string("CM_ENET_PORT");
    ENET_ADDR = attr_atom_from_string("CM_ENET_ADDR");
}

void DecodeAttrList(const char *attrs, char **in_transport, char **in_ip,
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

void ConnToolCallback(int dataID, const char *attrs, const char *data)
{
    char *IP = NULL, *transport = NULL, *hostname = NULL;
    int port = -1;
    DecodeAttrList(attrs, &transport, &IP, &hostname, &port);
    if (data)
    {
        std::cout << "Rank " << wrank << " callback data: " << data
                  << std::endl;
    }
    if (dataID == 0)
    {
        /* writer-side, prior to connection, giving info on listener network
         * parameters */
        if (!transport)
        {
            std::cout << "Rank " << wrank
                      << " Producer is listening on TCP/IP connection at IP "
                      << IP << " port " << port << std::endl;
        }
        else if (strcmp(transport, "enet") == 0)
        {
            std::cout << "Rank " << wrank
                      << " Producer is listening on UDP connection at IP " << IP
                      << " port " << port << std::endl;
        }
        else
        {
            std::cout << "Rank " << wrank
                      << " Producer Warning, unknown control network transport "
                         "operating"
                      << std::endl;
        }
        printf("\tSst connection tool waiting for connection...\n\n");
    }
    else if (dataID == 1)
    {
        /* reader-side, prior to connection, giving info on listener network
         * parameters */
        if (!transport)
        {
            std::cout << "Rank " << wrank
                      << " Consumer set up for TCP/IP connection at IP " << IP
                      << " port " << port << std::endl;
        }
        else if (strcmp(transport, "enet") == 0)
        {
            std::cout << "Rank " << wrank
                      << " Consumer set up for UDP connection at IP " << IP
                      << " port " << port << std::endl;
        }
        else
        {
            std::cout << "Rank " << wrank
                      << " Consumer Warning, unknown control network transport "
                         "operating"
                      << std::endl;
        }
    }
    else if (dataID == 2)
    {
        if (!transport)
        {
            std::cout
                << "Rank " << wrank
                << " Consumer Attempting TCP/IP connection to Producer at IP "
                << IP << " port " << port << std::endl;
        }
        else if (strcmp(transport, "enet") == 0)
        {
            std::cout
                << "Rank " << wrank
                << " Consumer Attempting UDP connection to Producer at IP "
                << IP << " port " << port << std::endl;
        }
        else
        {
            std::cout << "Rank " << wrank
                      << " Warning, unknown control network transport "
                         "operating"
                      << std::endl;
        }
    }
}

void do_connect()
{
    struct _SstParams Params;
    SstStream reader;
    memset(&Params, 0, sizeof(Params));
    // SSTSetNetworkCallback(ConnToolCallback);
    Params.RendezvousReaderCount = 1;
    // Params.ControlTransport = strdup("enet");
    Params.DataTransport = strdup("rdma");
    Params.OpenTimeoutSecs = 10;
    Params.RegistrationMethod = SstRegisterFile;

    reader = SstReaderOpen("libfabric_mpi_compatibility", &Params, &ADIOSComm);

    // MPI communication test (which also makes producers disconnect first,
    // consumer last)
    MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    if (reader)
    {
        std::cout << "Rank " << wrank << " Connection success, all is well!"
                  << std::endl;
        SstReaderClose(reader);
    }
    else
    {
        std::cout << "Rank " << wrank
                  << " This connection did not succeed.  Determine if IP "
                     "addresses in use are appropriate, or if firewalls or "
                     "other network-level artifacts might be causing a problem"
                  << std::endl;
    }
}

void do_listen()
{
    struct _SstParams Params;
    SstStream writer;
    memset(&Params, 0, sizeof(Params));
    Params.RegistrationMethod = SstRegisterFile;
    // SSTSetNetworkCallback(ConnToolCallback);
    Params.RendezvousReaderCount = 1;
    // Params.ControlTransport = "enet";
    Params.DataTransport = strdup("rdma");
    writer = SstWriterOpen("libfabric_mpi_compatibility", &Params, &ADIOSComm);
    std::cout << "Rank " << wrank << " Connection success, all is well!"
              << std::endl;

    // MPI communication test
    MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    SstWriterClose(writer);
}
