/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * solid fluid example
 *
 *  Created on: Oct 31, 2016
 *      Author: pnorbert
 */

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ADIOS_CPP.h"

using adiosFile = std::shared_ptr<adios::Engine>;

struct MYDATA
{
    int NX;
    double *t;
    std::vector<double> p;
};

const int N = 10;
struct MYDATA solid, fluid;
MPI_Comm comm = MPI_COMM_WORLD;
int rank, size;

void write_data(adiosFile writer, struct MYDATA &data)
{
    writer->Write("NX", &data.NX);
    writer->Write("rank", &rank);
    writer->Write("size", &size);
    writer->Write("temperature", data.t);
    writer->Write("pressure", data.p.data());
    // writer->Flush();  AdvanceStep()???
}

void write_checkpoint(adiosFile ckptfile, const struct MYDATA &solid,
                      const struct MYDATA &fluid)
{
    try
    {
        ckptfile->Write("solid/NX", &solid.NX);
        ckptfile->Write("solid/rank", &rank);
        ckptfile->Write("solid/size", &size);
        ckptfile->Write("solid/temperature", solid.t);
        ckptfile->Write("solid/pressure", &solid.p[0]);

        ckptfile->Write("fluid/NX", &fluid.NX);
        ckptfile->Write("fluid/rank", &rank);
        ckptfile->Write("fluid/size", &size);
        ckptfile->Write("fluid/temperature", fluid.t);
        ckptfile->Write("fluid/pressure", fluid.p.data());
        // ckptfile->AdvanceStep(); ??
    }
    catch (std::exception &e) // need to think carefully how to handle C++
                              // exceptions with MPI to avoid deadlocking
    {
        if (rank == 0)
        {
            std::cout << "ERROR: caught an exception from write_checkpoint\n";
            std::cout << e.what() << "\n";
        }
    }
}

void write_viz(std::shared_ptr<adios::Engine> vizstream, struct MYDATA &solid,
               struct MYDATA &fluid)
{
    // This stream is not associated with a group, so we must say for each write
    // which group to use
    // The output variable is re-defined inside as <groupname>/<varname>, unless
    // given as third string argument
    // An array variable's dimension definitions are also re-defined with
    // dimensions <groupname>/<dimensionvar>

    // vizstream->Write ("solid", "NX", &solid.NX);
    // vizstream->Write ("solid", "rank", &rank);
    // vizstream->Write ("solid", "size", &size);

    // write solid group's temperature simply as temperature, risking
    // overloading
    // the 'temperature' variable when
    // reading from a file

    // vizstream->Write ("solid", "temperature", "my/tempvarinfile", solid.t);

    // vizstream->Write ("fluid", "NX", &fluid.NX);
    // vizstream->Write ("fluid", "rank", &rank);
    // vizstream->Write ("fluid", "size", &size);

    // vizstream->Write ("fluid", "temperature", "temperature", fluid.t);

    // vizstream->Flush(); // flushes all data to disk; required operation
    // vizstream.AdvanceStep();
}

void compute(int it, struct MYDATA &solid, struct MYDATA &fluid)
{
    for (int i = 0; i < solid.NX; i++)
    {
        solid.t[i] = it * 100.0 + rank * solid.NX + i;
        solid.p[i] = it * 1000.0 + rank * solid.NX + i;
    }
    for (int i = 0; i < fluid.NX; i++)
    {
        fluid.t[i] = it * 200.0 + rank * fluid.NX + i;
        fluid.p[i] = it * 2000.0 + rank * fluid.NX + i;
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    solid.NX = N;
    solid.t = new double[N];
    solid.p = std::vector<double>(N);

    fluid.NX = N;
    fluid.t = new double[N];
    fluid.p = std::vector<double>(N);

    try
    {
        // ADIOS manager object creation. MPI must be initialized
        adios::ADIOS adios(comm, true);

        adios::Method &fileSettings =
            adios.DeclareMethod("Reusable"); // default engine is BP writer
        fileSettings.AddTransport("POSIX", "have_metadata_file=no");

        // Open a file with a Method which has selected a group and a transport
        // in
        // the XML.
        // "a" will append to an already existing file, "w" would create a new
        // file
        // Multiple writes to the same file work as append in this application
        // run
        // FIXME: how do we support Update to same step?

        auto solidfile = adios.Open("solid.bp", "w", comm, fileSettings);

        // "solid" is a method but incidentally also a group
        // Constructor only creates an object and what is needed there but does
        // not
        // open a stream/file
        // It can be used to initialize a staging connection if not declared
        // before
        // FIXME: which argument can be post-poned into Open() instead of
        // constructor?
        // solidfile.Open("solid.bp");

        // Open a file with a Method that has selected a group and an engine in
        // the
        // XML
        // The transport method(s) are (must be) associated with the engines
        // "a" will append to an already existing file, "w" would create a new
        // file
        // Multiple writes to the same file work as append in this application
        // run
        // FIXME: how do we support Update to same step?
        auto fluidfile = adios.Open("fluid.bp", "w", comm, fileSettings);

        auto checkpointFile =
            adios.Open("checkpoint.bp", "w", comm, fileSettings);

        // int ckptfile = adios.Open("checkpoint.bp", "checkpoint", "w", comm);
        // we do not open this here, but every time when needed in a function

        // Another output not associated with a single group, so that we can mix
        // variables to it
        // adios:handle vizstream = adios.Open( "stream.bp", comm, "w",
        // "STAGING",
        // "options to staging method");
        auto vizstream = adios.Open("stream.bp", "w", comm, fileSettings);

        // This creates an empty group inside, and we can write all kinds of
        // variables to it

        // Get Monitor info
        std::ofstream logStream("info_" + std::to_string(rank) + ".log");
        adios.MonitorVariables(logStream);

        int checkPointSteps = 10;

        for (int it = 1; it <= 100; it++)
        {
            compute(it, solid, fluid);

            write_data(solidfile, solid);
            write_data(fluidfile, fluid);

            if (it % checkPointSteps == 0)
            {
                write_checkpoint(checkpointFile, solid, fluid);

                MPI_Barrier(comm);
                if (rank == 0)
                {
                    std::cout
                        << "New checkpoint step, current = " << checkPointSteps
                        << "\n";
                    std::cin >> checkPointSteps;
                    MPI_Bcast(&checkPointSteps, 1, MPI_INT, 0, comm);
                }
                MPI_Barrier(comm);
            }

            write_viz(vizstream, solid, fluid);

            MPI_Barrier(comm);
            if (rank == 0)
                printf("Timestep %d written\n", it);
        }

        solidfile->Close();
        fluidfile->Close();
        vizstream->Close();

        // need barrier before we destroy the ADIOS object here automatically
        MPI_Barrier(comm);
        if (rank == 0)
            printf("Finalize adios\n");
    }
    catch (std::exception &e) // need to think carefully how to handle C++
                              // exceptions with MPI to avoid deadlocking
    {
        if (rank == 0)
        {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }

    delete[] solid.t;
    delete[] fluid.t;

    if (rank == 0)
        printf("Finalize MPI\n");
    MPI_Finalize();
    return 0;
}
