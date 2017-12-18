/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIReader.h
 * Class to exchange data using MPI between Writer and Reader
 *  partition of an application
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InSituMPIFunctions.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread> // sleep_for

namespace adios2
{

namespace insitumpi
{

std::vector<int> FindPeers(MPI_Comm comm, std::string name, bool amIWriter)
{
    std::vector<int> mylist;   // 'our' ranks in the world comm
    std::vector<int> peerlist; // 'their' ranks in the world comm

    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

    if (wnproc == nproc)
    {
        return peerlist;
    }

    mylist.reserve(nproc);
    MPI_Gather(&wrank, 1, MPI_INT, mylist.data(), 1, MPI_INT, 0, comm);

    std::string ofName;
    std::string ifName;
    if (amIWriter)
    {
        ofName = name + "_insitumpi_writers";
        ifName = name + "_insitumpi_readers";
    }
    else
    {
        ofName = name + "_insitumpi_readers";
        ifName = name + "_insitumpi_writers";
    }

    int pnproc; // peer nproc

    if (!rank)
    {
        std::ofstream outf(ofName, std::ios::out | std::ios::binary);
        outf.write((char *)&nproc, sizeof(int));
        outf.write((char *)mylist.data(), nproc * sizeof(int));
        outf.close();
        // std::cout << "rank " << wrank << ": Created info file " << ofName
        //          << std::endl;

        std::ifstream inpf;
        inpf.open(ifName, std::ios::in | std::ios::binary);
        while (!inpf.is_open())
        {
            // std::cout << "rank " << wrank
            //          << ": Waiting for peers to create info file..."
            //          << std::endl;
            std::chrono::milliseconds timespan(500);
            std::this_thread::sleep_for(timespan);
            inpf.open(ifName, std::ios::in | std::ios::binary);
        }
        inpf.read((char *)&pnproc, sizeof(int));
        // std::cout << "rank " << wrank << ": #pnproc = " << pnproc <<
        // std::endl;
        peerlist.resize(pnproc);
        inpf.read((char *)peerlist.data(), pnproc * sizeof(int));
        inpf.close();
        // std::cout << "rank " << wrank << ": Read peer info file " << ifName
        //          << std::endl;
    }

    MPI_Bcast(&pnproc, 1, MPI_INT, 0, comm);
    if (rank)
    {
        peerlist.resize(pnproc);
    }
    MPI_Bcast(peerlist.data(), pnproc, MPI_INT, 0, comm);

    // std::cout << "rank " << wrank << ": FindPeers " << wrank
    //          << (amIWriter ? " Writer" : " Reader")
    //          << ". #peers=" << peerlist.size() << " #pnproc=" << pnproc
    //          << std::endl;

    if (!rank)
    {
        std::remove(ifName.c_str());
    }

    return peerlist;
}

} // end namespace insitumpi

} // end namespace adios2
