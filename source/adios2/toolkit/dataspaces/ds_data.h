/*
 * ds_data.h
 *
 *  Created on: Dec 12, 2018
 *      Author: subedip
 */

#ifndef _DS_DATA_H_
#define _DS_DATA_H_

#ifndef _SYS_TYPES_H_
#include <sys/types.h>
#endif
#include "mpi.h"

#define MAX_NUM_OF_STREAMS 20
#define MAX_DS_NDIM 10

struct adios_dspaces_stream_info {
    char *name;         // file name passed in adios_open()
    int  time_index;    // versioning, start from 0
    int  iam_rank0;     // 1: current process has been rank 0 for this stream
                        //    rank 0 of communicator for this file does extra work in finalize
};
struct adios_ds_data_struct
{
    int rank;   // dataspaces rank or MPI rank if MPI is available
    int peers;  // from xml parameter or group communicator
    int appid;  // from xml parameter or 1
    int n_writes; // how many times adios_write has been called
    MPI_Comm mpi_comm; // for use in open..close
    MPI_Comm mpi_comm_init; // for use in init/finalize
};


#endif /* _DS_DATA_H_ */
