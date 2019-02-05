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
#include "mpi.h"
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


// Save data for each adios group using this method (multiple ones may exist)


/**********************************************************************************
* Functions to manage the set of "files" or streams opened for all ADIOS groups
* We store all names (with version info, and responsible "rank 0" master process id
* to be used in adios_dataspaces_finalize().
**********************************************************************************/
//#define MAX_NUM_OF_STREAMS 20
//static struct adios_dspaces_stream_info stream_info[MAX_NUM_OF_STREAMS];
//static int  num_of_streams = 0; // how many files do we have with this method (in total for entire run)
                                // i.e. this variable never decreases
/*
static void free_dspaces_stream_info()
{
    int i;
    struct adios_dspaces_stream_info *info;
    for (i = 0; i < num_of_streams; i++) {
        info = &stream_info[i];
        if (info->name) {
            free(info->name);
        }
        info->name = NULL;
        info->time_index = -1; // time_index (dataspaces versioning) starts from 0
        info->iam_rank0 = 0;
    }
    return;
}

static struct adios_dspaces_stream_info* lookup_dspaces_stream_info(const char* fname)
{
    int i;
    // search from last to first
    for (i = num_of_streams-1; i >= 0; i--)
    {
        if (stream_info[i].name != NULL &&
            strcmp(stream_info[i].name, fname) == 0)
        {
            fprintf (stderr, "Stream %s is going to be continued... num_of_streams=%d\n",
                fname, num_of_streams);
            return &stream_info[i];
        }
    }
    // not found, add new opened stream to list
    if (num_of_streams < MAX_NUM_OF_STREAMS)
    {
        fprintf (stderr, "New stream %s added.  num_of_streams=%d\n",
                fname, num_of_streams);
        i = num_of_streams;
        num_of_streams++;
        stream_info[i].name = strdup(fname);
        stream_info[i].time_index = -1;
        return &stream_info[i];
    }
    else
    {
        // we cannot add more
        fprintf (stderr,
                     "ERROR: Max %d different files can be written by one application "
                     "using the same ADIOS group when using the DATASPACES method.\n",
                     MAX_NUM_OF_STREAMS);
    }

    return NULL;
}
*/

static int connect_to_dspaces (struct adios_ds_data_struct * md, MPI_Comm comm)
{
    int ret = 0;
    int num_peers;

    if (!globals_adios_is_dataspaces_connected()) {

        MPI_Comm_rank (comm, &(md->rank));
        MPI_Comm_size (comm, &num_peers);

        // Application ID should be set by the application calling adios_set_application_id()
        int was_set;
        md->appid = globals_adios_get_application_id (&was_set);
        if (!was_set)
            md->appid = 1;

        fprintf (stderr, "adios_dataspaces: rank=%d connect to DATASPACES, peers=%d, appid=%d \n",
                md->rank, num_peers, md->appid);

        //Init the dart client
        ret = dspaces_init (num_peers, md->appid, &md->mpi_comm_init, NULL);
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
    md->mpi_comm = *(MPI_Comm *)comm;
    md->mpi_comm_init = *(MPI_Comm *)comm;


    connect_to_dspaces (md, comm);
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
	        err = dspaces_init(nproc, appid, &comm, NULL);
	        if (err < 0) {
	            fprintf (stderr, "Failed to connect with DATASPACES\n");
	            return err;
	        }
	    }
	    globals_adios_set_dataspaces_connected_from_reader();
	    md->rank = rank;
	    md->mpi_comm = comm;
	    md->appid = appid;

	    fprintf(stderr, "Connected to DATASPACES\n");
	    return 0;

}
