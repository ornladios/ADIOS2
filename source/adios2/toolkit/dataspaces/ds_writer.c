/*
 * ds_writer.c
 *
 *  Created on: Jan 4, 2019
 *      Author: subedip
 */
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "adios2/ADIOSConfig.h"
#include <mpi.h>
#include "dataspaces.h"
#include "ds.h"

static int adios_dataspaces_initialized = 0;
#define MAX_DS_NAMELEN 128


// count the number of inits/finalizes (one per adios group using this method
static unsigned int number_of_inits = 0;

static int globals_adios_appid = -1;
static int globals_adios_was_set = 0;
void globals_adios_set_application_id (int id)
{
    globals_adios_appid = id;
    globals_adios_was_set = 1;
}

int globals_adios_get_application_id (int *was_set)
{
    *was_set = globals_adios_was_set;
    return globals_adios_appid;
}

enum DATASPACES_CONNECTION { dataspaces_disconnected = 0,
                       dataspaces_connected_from_reader = 1,
                       dataspaces_connected_from_writer = 2,
                       dataspaces_connected_from_both = 3
                     };
static enum DATASPACES_CONNECTION globals_adios_connected_to_dataspaces = dataspaces_disconnected;

void globals_adios_set_dataspaces_connected_from_reader()
{
    if (globals_adios_connected_to_dataspaces == dataspaces_disconnected)
        globals_adios_connected_to_dataspaces = dataspaces_connected_from_reader;
    else if (globals_adios_connected_to_dataspaces == dataspaces_connected_from_writer)
        globals_adios_connected_to_dataspaces = dataspaces_connected_from_both;
}
void globals_adios_set_dataspaces_disconnected_from_reader()
{
    if (globals_adios_connected_to_dataspaces == dataspaces_connected_from_reader)
        globals_adios_connected_to_dataspaces = dataspaces_disconnected;
    else if (globals_adios_connected_to_dataspaces == dataspaces_connected_from_both)
        globals_adios_connected_to_dataspaces = dataspaces_connected_from_writer;
}
void globals_adios_set_dataspaces_connected_from_writer()
{
    if (globals_adios_connected_to_dataspaces == dataspaces_disconnected)
        globals_adios_connected_to_dataspaces = dataspaces_connected_from_writer;
    else if (globals_adios_connected_to_dataspaces == dataspaces_connected_from_reader)
        globals_adios_connected_to_dataspaces = dataspaces_connected_from_both;
}
void globals_adios_set_dataspaces_disconnected_from_writer()
{
    if (globals_adios_connected_to_dataspaces == dataspaces_connected_from_writer)
        globals_adios_connected_to_dataspaces = dataspaces_disconnected;
    else if (globals_adios_connected_to_dataspaces == dataspaces_connected_from_both)
        globals_adios_connected_to_dataspaces = dataspaces_connected_from_reader;
}
int  globals_adios_is_dataspaces_connected()
{
    return (globals_adios_connected_to_dataspaces != dataspaces_disconnected);
}
int  globals_adios_is_dataspaces_connected_from_reader()
{
    return (globals_adios_connected_to_dataspaces == dataspaces_connected_from_reader ||
            globals_adios_connected_to_dataspaces == dataspaces_connected_from_both);
}
int  globals_adios_is_dataspaces_connected_from_writer()
{
    return (globals_adios_connected_to_dataspaces == dataspaces_connected_from_writer ||
            globals_adios_connected_to_dataspaces == dataspaces_connected_from_both);
}
int  globals_adios_is_dataspaces_connected_from_both()
{
    return (globals_adios_connected_to_dataspaces == dataspaces_connected_from_both);
}


static int connect_to_dspaces (struct adios_ds_data_struct * md, MPI_Comm comm)
{
    int ret = 0;
    int num_peers;
    fprintf(stderr, "Before checking if dspaces is connected\n");

    if (!globals_adios_is_dataspaces_connected()) {

    	 fprintf(stderr, "DataSpaces is not connected\n");
        MPI_Comm_rank (comm, &(md->rank));
        fprintf(stderr, "After rank copy\n");
        MPI_Comm_size (comm, &num_peers);
        fprintf(stderr, "After MPI functions\n");

        // Application ID should be set by the application calling adios_set_application_id()
        int was_set;
        md->appid = globals_adios_get_application_id (&was_set);
        if (!was_set)
            md->appid = 1;

        //Init the dart client
        ret = dspaces_init (num_peers, md->appid, &md->mpi_comm_init, NULL);
        fprintf(stderr, "After dspaces_init");
        if (ret) {
            fprintf (stderr, "adios_dataspaces: rank=%d Failed to connect to DATASPACES: err=%d,  rank=%d\n", ret, md->rank);
            return ret;
        }
/*
#if ! HAVE_MPI
        dspaces_rank (&(md->rank));
        dspaces_peers (&(md->peers));
#else*/

        md->peers = num_peers;
//#endif

        fprintf (stderr, "adios_dataspaces: rank=%d connected to DATASPACES: peers=%d\n", md->rank, md->peers);
    }

    globals_adios_set_dataspaces_connected_from_writer();
    return ret;
}

void adios_dataspaces_init (void* comm, DsData* md)
{
    if (!adios_dataspaces_initialized)
    {
        adios_dataspaces_initialized = 1;
    }

    int index, i;
    char temp[64];

    //Init the static data structure
    md->peers = 1;
    md->appid = -1;
    md->n_writes = 0;
    md->rank = 0;
    md->mpi_comm = *(MPI_Comm *)comm;
    md->mpi_comm_init = *(MPI_Comm *)comm;


    connect_to_dspaces (md, *(MPI_Comm *)comm);
    number_of_inits++;
    fprintf (stderr,"adios_dataspaces_init: called the %d. time\n", number_of_inits);

}

void adios_dataspaces_open (char* fname, DsData* md)
{
    // if we have MPI and a communicator, we can get the exact size of this application
    // that we need to tell DATASPACES
    MPI_Comm_rank (md->mpi_comm, &(md->rank));
    MPI_Comm_size (md->mpi_comm, &(md->peers));


    fprintf (stderr, "adios_dataspaces_open: rank=%d call write lock...\n", md->rank);
    dspaces_lock_on_write (fname, &md->mpi_comm);
    fprintf (stderr, "adios_dataspaces_open: rank=%d got write lock\n", md->rank);
}

int adios_read_dataspaces_init (void* comm, DsData* md)
{
	int  nproc;
	int  rank, err;
	int  appid, was_set;
	MPI_Comm_rank(*(MPI_Comm *)comm, &rank);
	MPI_Comm_size(*(MPI_Comm *)comm, &nproc);

	/* Connect to DATASPACES, but only if we are not yet connected (from Write API) */
	    if (!globals_adios_is_dataspaces_connected()) {
	        appid = globals_adios_get_application_id (&was_set);
	        if (!was_set)
	            appid = 2;
	        fprintf(stderr, "-- %s, rank %d: connect to dataspaces with nproc=%d and appid=%d\n",
	                    __func__, rank, nproc, appid);
	        err = dspaces_init(nproc, appid, (MPI_Comm *)comm, NULL);
	        if (err < 0) {
	            fprintf (stderr, "Failed to connect with DATASPACES\n");
	            return err;
	        }
	    }
	    globals_adios_set_dataspaces_connected_from_reader();
	    md->rank = rank;
	    md->mpi_comm = *(MPI_Comm *)comm;
	    md->appid = appid;

	    fprintf(stderr, "Connected to DATASPACES\n");
	    return 0;

}
