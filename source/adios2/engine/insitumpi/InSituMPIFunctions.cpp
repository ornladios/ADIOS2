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

std::vector<int> FindPeers(const MPI_Comm comm, const std::string &name,
                           const bool amIWriter, const MPI_Comm commWorld)
{
    std::vector<int> mylist;   // 'our' ranks in the world comm
    std::vector<int> peerlist; // 'their' ranks in the world comm

    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    int wrank, wnproc;
    MPI_Comm_rank(commWorld, &wrank);
    MPI_Comm_size(commWorld, &wnproc);

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
    std::string ofNameTmp = ofName + ".tmp";

    int pnproc; // peer nproc

    if (!rank)
    {
        std::ofstream outf(ofNameTmp, std::ios::out | std::ios::binary);
        outf.write(reinterpret_cast<char *>(&nproc), sizeof(int));
        outf.write(reinterpret_cast<char *>(mylist.data()),
                   nproc * sizeof(int));
        outf.close();
        int result = std::rename(ofNameTmp.c_str(), ofName.c_str());
        if (result != 0)
        {
            std::cerr << "ADIOS2 FindPeers error renaming file " << ofNameTmp
                      << " to " << ofNameTmp << std::endl;
        }
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

std::vector<int> AssignPeers(const int rank, const int nproc,
                             const std::vector<int> &allPeers)
{
    int nAllPeers = allPeers.size();

    std::vector<int> directPeers;

    std::cout << "AssignPeers: nAllPeers = " << nAllPeers
              << ", nproc = " << nproc << std::endl;

    if (nproc == nAllPeers)
    {
        // one-to-one direct assignment
        directPeers.push_back(allPeers[rank]);
    }
    else if (nproc < nAllPeers)
    {
        int nDirectPeers = nAllPeers / nproc;
        int startPos = rank * nDirectPeers;
        if (rank < nAllPeers % nproc)
        {
            nDirectPeers++;
            startPos += rank;
        }
        else
        {
            startPos += nAllPeers % nproc;
        }
        auto first = allPeers.begin() + startPos;
        auto last = allPeers.begin() + startPos + nDirectPeers;
        directPeers.insert(directPeers.begin(), first, last);
        // std::cout << "rank " << rank << ": nproc=" << nproc
        //          << " npeers=" << nDirectPeers << " start=" << startPos
        //          << " peers=[";
        // for (const auto i : directPeers)
        //    std::cout << i << " ";
        // std::cout << "]" << std::endl;
    }
    else if (nproc > nAllPeers)
    {
        int base = nproc / nAllPeers;
        int peerRank = -1;
        int pos = 0;
        // std::cout << "rank " << rank << ": nproc=" << nproc
        //          << " npeers=" << nAllPeers << " base=" << base << std::endl;

        while (peerRank < nAllPeers && pos <= rank)
        {
            peerRank++;
            pos += base;
            if (peerRank < nproc % nAllPeers)
            {
                pos++;
            }
            // std::cout << " peerRank=" << peerRank << " pos=" << pos
            //          << std::endl;
        }
        // std::cout << " final peerRank=" << peerRank << " pos=" << pos
        //          << std::endl;
        directPeers.push_back(allPeers[peerRank]);
    }
    return directPeers;
}

int ConnectDirectPeers(const MPI_Comm commWorld, const bool IAmSender,
                       const bool IAmWriterRoot, const int globalRank,
                       const std::vector<int> &peers)
{
    int token = (IAmWriterRoot ? 1 : 0);
    int writeRootGlobalRank = -1;
    MPI_Status status;
    for (const auto peerRank : peers)
    {
        if (IAmSender)
        {
            // std::cout << " Send from " << rank << " to " << peerRank
            //          << std::endl;
            MPI_Send(&token, 1, MPI_INT, peerRank, MpiTags::Connect, commWorld);
        }
        else
        {
            // std::cout << " Recv from " << peerRank << " by " << rank
            //          << std::endl;
            MPI_Recv(&token, 1, MPI_INT, peerRank, MpiTags::Connect, commWorld,
                     &status);
            if (token == 1)
                writeRootGlobalRank = peerRank;
        }
    }
    return writeRootGlobalRank;
}

} // end namespace insitumpi

} // end namespace adios2
