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

#include "ADIOS.h"


struct MYDATA {
    int NX;
    double *t;
    std::vector<double> p;
};

const int N = 10;
struct MYDATA solid, fluid;
MPI_Comm    comm=MPI_COMM_WORLD;
int         rank, size;


void write_data (adios::ADIOS adios, struct MYDATA &data, unsigned int outfile)
{
    adios.Write (outfile, "NX", &data.NX);
    adios.Write (outfile, "rank", &rank);
    adios.Write (outfile, "size", &size);
    adios.Write (outfile, "temperature", data.t);
    adios.Write (outfile, "pressure", data.p);
    //adios.Write (outfile, true); // true: advance step, this is default value
    adios.Write (outfile);
}

void write_checkpoint (adios::ADIOS adios, struct MYDATA &solid, struct MYDATA &fluid)
{
    try {
        // Open an output for a Group
        // a transport or an engine should be associated with the group
        int ckptfile = adios.Open("checkpoint.bp", "checkpoint", "w", comm);

        adios.Write (ckptfile, "solid/NX", &solid.NX);
        adios.Write (ckptfile, "solid/rank", &rank);
        adios.Write (ckptfile, "solid/size", &size);
        adios.Write (ckptfile, "solid/temperature", solid.t);
        adios.Write (ckptfile, "solid/pressure", solid.p);

        adios.Write (ckptfile, "fluid/NX", &fluid.NX);
        adios.Write (ckptfile, "fluid/rank", &rank);
        adios.Write (ckptfile, "fluid/size", &size);
        adios.Write (ckptfile, "fluid/temperature", fluid.t);
        adios.Write (ckptfile, "fluid/pressure", fluid.p);

        adios.Write(ckptfile);
        adios.Close(ckptfile); // Should this do Write() if user misses or should we complain?
    }
    catch ( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        if( rank == 0 )
        {
            std::cout << e.what() << "\n";
        }
    }
}

void write_viz (adios::ADIOS adios, struct MYDATA &solid, struct MYDATA &fluid, unsigned int vizstream)
{
    // This stream is not associated with a group, so we must say for each write which group to use
    // The output variable is re-defined inside as <groupname>/<varname>, unless given as third string argument
    // An array variable's dimension definitions are also re-defined with dimensions <groupname>/<dimensionvar>
    adios.Write (vizstream, "solid", "NX", &solid.NX);
    adios.Write (vizstream, "solid", "rank", &rank);
    adios.Write (vizstream, "solid", "size", &size);
    // write solid group's temperature simply as temperature, risking overloading the 'temperature' variable when
    // reading from a file
    adios.Write (vizstream, "solid", "temperature", "my/tempvarinfile", solid.t);

    adios.Write (vizstream, "fluid", "NX", &fluid.NX);
    adios.Write (vizstream, "fluid", "rank", &rank);
    adios.Write (vizstream, "fluid", "size", &size);

    adios.Write (vizstream, "fluid", "temperature", "temperature", fluid.t);

    adios.Write(vizstream); // flushes all data to disk; required operation
    vizstream.AdvanceStep();
}

void compute (int it,  struct MYDATA &solid, struct MYDATA &fluid)
{
    for (int i = 0; i < solid.NX; i++)
    {
        solid.t[i] = it*100.0 + rank*solid.NX + i;
        solid.p[i] = it*1000.0 + rank*solid.NX + i;
    }
    for (int i = 0; i < fluid.NX; i++)
    {
        fluid.t[i] = it*200.0 + rank*fluid.NX + i;
        fluid.p[i] = it*2000.0 + rank*fluid.NX + i;
    }
}

int main( int argc, char* argv [] )
{
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    solid.NX = N;
    solid.t = new double(N);
    solid.p = std::vector<double>(N);

    fluid.NX = N;
    fluid.t = new double(N);
    fluid.p = std::vector<double>(N);

    try
    {
        // ADIOS manager object creation. MPI must be initialized
        adios::ADIOS adios( "globalArrayXML.xml", comm, true );

        // Open a file with a Method which has selected a group and a transport in the XML.
        // "a" will append to an already existing file, "w" would create a new file
        // Multiple writes to the same file work as append in this application run
        // FIXME: how do we support Update to same step?

        int solidfile = adios.Open("solid.bp", "solid", "a", comm); // "solid" is a method but incidentally also a group
        // Constructor only creates an object and what is needed there but does not open a stream/file
        // It can be used to initialize a staging connection if not declared before
        // FIXME: which argument can be post-poned into Open() instead of constructor?
        //solidfile.Open("solid.bp");


        // Open a file with a Method that has selected a group and an engine in the XML
        // The transport method(s) are (must be) associated with the engines
        // "a" will append to an already existing file, "w" would create a new file
        // Multiple writes to the same file work as append in this application run
        // FIXME: how do we support Update to same step?
        int fluidfile = adios.Open("fluid.bp", "fluid", "a", comm);

        //int ckptfile = adios.Open("checkpoint.bp", "checkpoint", "w", comm);
        // we do not open this here, but every time when needed in a function

        // Another output not associated with a single group, so that we can mix variables to it
        //adios:handle vizstream = adios.Open( "stream.bp", comm, "w", "STAGING", "options to staging method");
        int vizstream = adios.Open( "stream.bp", comm, "w", "groupless");

        // This creates an empty group inside, and we can write all kinds of variables to it

        //Get Monitor info
        std::ofstream logStream( "info_" + std::to_string(rank) + ".log" );
        adios.MonitorGroups( logStream );

        for (int it = 1; it <= 100; it++)
        {
            compute (it, solid, fluid);

            write_data(adios, solid, solidfile);
            write_data(adios, fluid, fluidfile);

            if (it%10 == 0) {
                write_checkpoint (adios, solid, fluid);
            }

            write_viz(adios, solid, fluid, vizstream);

            MPI_Barrier (comm);
            if (rank==0) printf("Timestep %d written\n", it);
        }

        adios.Close(solidfile);
        adios.Close(fluidfile);
        adios.Close(vizstream);

        // need barrier before we destroy the ADIOS object here automatically
        MPI_Barrier (comm);
        if (rank==0) printf("Finalize adios\n");
    }
    catch( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        if( rank == 0 )
        {
            std::cout << e.what() << "\n";
        }
    }

    delete[] solid.t;
    delete[] fluid.t;

    if (rank==0) printf("Finalize MPI\n");
    MPI_Finalize ();
    return 0;
}
