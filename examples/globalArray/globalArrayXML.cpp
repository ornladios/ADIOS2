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

int main( int argc, char* argv [] )
{
    int         rank, size;
    const int   NX = 10;
    double      t[NX];
    std::vector<double> p(NX);
    MPI_Comm    comm=MPI_COMM_WORLD;

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    try
    {
        // ADIOS manager object creation. MPI must be initialized
        adios::ADIOS adios( "globalArrayXML.xml", comm, true );

        //Get Monitor info
        std::ofstream logStream( "info_" + std::to_string(rank) + ".log" );
        adios.MonitorGroups( logStream );

        for (int it = 1; it <= 13; it++)
        {

            for (int i = 0; i < NX; i++)
            {
                t[i] = it*100.0 + rank*NX + i;
                p[i] = it*1000.0 + rank*NX + i;
            }

            if (it==1)
                adios.Open("arrays", "globalArray.bp", "w");
            else
                adios.Open("arrays", "globalArray.bp", "a");

            //uint64_t    adios_groupsize, adios_totalsize;
            // adios_groupsize = 4 + 4 + 4 + 2*NX*sizeof(double);
            // adios_totalsize = adios.GroupSize("arrays", adios_groupsize);

            adios.Write ("arrays", "NX", &NX);
            adios.Write ("arrays", "size", &size);
            adios.Write ("arrays", "rank", &rank);
            adios.Write ("arrays", "temperature", t);
            adios.Write ("arrays", "pressure", &p);

            adios.Close ("arrays");
            MPI_Barrier (comm);
            if (rank==0) printf("Timestep %d written\n", it);
        }
        MPI_Barrier (comm);
        // need barrier before we destroy the ADIOS object here automatically
        if (rank==0) printf("Finalize adios\n");
    }
    catch( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        if( rank == 0 )
        {
            std::cout << e.what() << "\n";
        }
    }

    if (rank==0) printf("Finalize MPI\n");
    MPI_Finalize ();
    return 0;
}
