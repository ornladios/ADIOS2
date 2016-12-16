/*
 * globalArrayXML.cpp
 *
 *  Created on: Oct 31, 2016
 *      Author: pnorbert
 */

#include <stdexcept>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "public/ADIOS.h"


struct MYDATA {
    int NX;
    double *t;
    std::vector<double> p;
};

const int N = 10;
struct MYDATA solid, fluid;
MPI_Comm    comm=MPI_COMM_WORLD;
int         rank, size;

void read_ckpt (struct MYDATA &solid, struct MYDATA &fluid)
{
    try {
        // Open an input which was written with an expected Method
        // The writing transport method is associated with the group in the XML
        // ADIOS pairs that with the corresponding read transport
        // "r" is required to indicate we are reading
        adios::ADIOS_INPUT ckptfile( comm, "r", "checkpoint");
        ckptfile.Open("ckpt.bp");

        // Note: we only see a single step in the input but the checkpoint has
        // only one step anyway. This makes this code simple

        // simple immediate read of a scalar
        ckptfile.ReadScalar ("solid/NX", &solid.NX);
        // solid.NX is filled at this point

        // scheduled version of read of another scalar
        ckptfile.ScheduleRead ("fluid/NX", &fluid.NX);
        ckptfile.Read(); // perform reading now
        // fluid.NX is filled at this point

        solid.t = new double(solid.NX);
        solid.p = std::vector<double>(solid.NX);

        fluid.t = new double(fluid.NX);
        fluid.p = std::vector<double>(fluid.NX);

        adios::ADIOS_SELECTION_WRITEBLOCK sel(rank);
        ckptfile.ScheduleRead (sel, "solid/temperature", solid.t);
        ckptfile.ScheduleRead (sel, "solid/pressure",    solid.p);
        ckptfile.ScheduleRead (sel, "fluid/temperature", fluid.t);
        // force checking if the allocated space equals to the size of the selection:
        ckptfile.ScheduleRead (sel, "fluid/pressure",    fluid.p, fluid.NX*sizeof(double));
        ckptfile.Read(true); // true: blocking read
        ckptfile.Close(); // Should this do Read() if user misses or should we complain?
    }
    catch ( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        if( rank == 0 )
        {
            std::cout << e.what() << "\n";
        }
    }
}

void read_solid (struct MYDATA &solid)
{
    float timeout_sec = 1.0;
    int retval = 0;

    // Open a file for input, no group defined
    // A reading transport should be defined if not file based
    // "r" is required to indicate we are reading
    adios::ADIOS_INPUT solidfile( comm, "r");

    try {
        solidfile.Open("solid.bp"); //, ADIOS_LOCKMODE_NONE, timeout_sec);

        /* process file here... */
        const adios::ADIOS_VARINFO &v = solidfile.InqVar ("temperature");
        v.GetBlockInfo();

        printf ("ndim = %d\n",  v.ndim);
        //printf ("nsteps = %d\n",  v.nsteps);
        printf ("dims[%" PRIu64 "][%" PRIu64 "]\n",  v.dims[0], v.dims[1]);

        uint64_t slice_size = v.dims[0]/size;

        if (rank == size-1)
            slice_size = slice_size + v.dims[0]%size;

        start[0] = rank * slice_size;
        count[0] = slice_size;
        start[1] = 0;
        count[1] = v.dims[1];

        data = malloc (slice_size * v.dims[1] * 8);

        adios::ADIOS_SELECTION_BOUNDINGBOX sel(v.ndim, start, count);

        /* Processing loop over the steps (we are already in the first one) */
        while (adios_errno != err_end_of_stream) {
            steps++; // steps start counting from 1

            solidfile.ScheduleRead (sel, "temperature", solid.t);
            solidfile.Read(true);

            if (rank == 0)
                printf ("--------- Step: %d --------------------------------\n",
                        solidfile.getCurrentStep());

            printf("rank=%d: [0:%" PRIu64 ",0:%" PRIu64 "] = [", rank, v.dims[0], v.dims[1]);
            for (i = 0; i < slice_size; i++) {
                printf (" [");
                for (j = 0; j < v.dims[1]; j++) {
                    printf ("%g ", *((double *)data + i * v.dims[1] + j));
                }
                printf ("]");
            }
            printf (" ]\n\n");

            // advance to 1) next available step with 2) blocking wait
            solidfile.AdvanceStep (false, timeout_sec);
            if (adios_errno == err_step_notready)
            {
                printf ("rank %d: No new step arrived within the timeout. Quit. %s\n",
                        rank, adios_errmsg());
                break; // quit while loop
            }

        }
        solidfile.Close();

    }
    catch  ( std::exception& e )
    {
        if (adios_errno == err_file_not_found)
        {
            printf ("rank %d: Stream not found after waiting %f seconds: %s\n",
                    rank, timeout_sec, adios_errmsg());
            retval = adios_errno;
        }
        else if (adios_errno == err_end_of_stream)
        {
            printf ("rank %d: Stream terminated before open. %s\n", rank, adios_errmsg());
            retval = adios_errno;
        }
        else if (f == NULL) {
            printf ("rank %d: Error at opening stream: %s\n", rank, adios_errmsg());
            retval = adios_errno;
        }
    }
}

void read_fluid (struct MYDATA &fluid)
{
    float timeout_sec = 1.0;
    int retval = 0;

    // Open a file for input, no group defined
    // A reading transport should be defined if not file based
    // "r" is required to indicate we are reading
    adios::ADIOS_INPUT fluidfile( comm, "r");

    try {
        fluidfile.Open("fluid.bp"); //, ADIOS_LOCKMODE_NONE, timeout_sec);

        /* process file here... */
        const adios::ADIOS_VARINFO &v = fluidfile.InqVar ("temperature");
        v.GetBlockInfo();

        printf ("ndim = %d\n",  v.ndim);
        //printf ("nsteps = %d\n",  v.nsteps);
        printf ("dims[%" PRIu64 "][%" PRIu64 "]\n",  v.dims[0], v.dims[1]);

        uint64_t slice_size = v.dims[0]/size;

        if (rank == size-1)
            slice_size = slice_size + v.dims[0]%size;

        start[0] = rank * slice_size;
        count[0] = slice_size;
        start[1] = 0;
        count[1] = v.dims[1];

        data = malloc (slice_size * v.dims[1] * 8);

        adios::ADIOS_SELECTION_BOUNDINGBOX sel(v.ndim, start, count);

        /* Processing loop over the steps (we are already in the first one) */
        while (adios_errno != err_end_of_stream) {
            steps++; // steps start counting from 1

            fluidfile.ScheduleRead (sel, "temperature", fluid.t);
            fluidfile.Read(true);

            if (rank == 0)
                printf ("--------- Step: %d --------------------------------\n",
                        fluidfile.getCurrentStep());

            printf("rank=%d: [0:%" PRIu64 ",0:%" PRIu64 "] = [", rank, v.dims[0], v.dims[1]);
            for (i = 0; i < slice_size; i++) {
                printf (" [");
                for (j = 0; j < v.dims[1]; j++) {
                    printf ("%g ", *((double *)data + i * v.dims[1] + j));
                }
                printf ("]");
            }
            printf (" ]\n\n");

            // advance to 1) next available step with 2) blocking wait
            fluidfile.AdvanceStep (false, timeout_sec);
            if (adios_errno == err_step_notready)
            {
                printf ("rank %d: No new step arrived within the timeout. Quit. %s\n",
                        rank, adios_errmsg());
                break; // quit while loop
            }

        }
        fluidfile.Close();

    }
    catch  ( std::exception& e )
    {
        if (adios_errno == err_file_not_found)
        {
            printf ("rank %d: Stream not found after waiting %f seconds: %s\n",
                    rank, timeout_sec, adios_errmsg());
            retval = adios_errno;
        }
        else if (adios_errno == err_end_of_stream)
        {
            printf ("rank %d: Stream terminated before open. %s\n", rank, adios_errmsg());
            retval = adios_errno;
        }
        else if (f == NULL) {
            printf ("rank %d: Error at opening stream: %s\n", rank, adios_errmsg());
            retval = adios_errno;
        }
    }
}


int main (int argc, char ** argv)
{
    int         i, j;
    ADIOS_FILE * f;
    ADIOS_VARINFO * v;
    ADIOS_SELECTION * sel;
    int steps = 0;
    int retval = 0;
    float timeout_sec = 1.0;

    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    try
    {
        // ADIOS manager object creation. MPI must be initialized
        adios::ADIOS adios( "globalArrayXML.xml", comm, true );

        read_ckpt(solid, fluid);

        read_solid(solid);

        read_fluid(fluid);
    }
    catch( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        if( rank == 0 )
        {
            std::cout << e.what() << "\n";
        }
    }
    if (rank==0)
        printf ("We have processed %d steps\n", steps);

    free (data);
    MPI_Finalize ();

    return retval;

}
