/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMpiHandshake.cpp
 *
 *  Created on: Mar 1, 2020
 *      Author: Jason Wang
 */

#include "adiosMpiHandshake.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>
#include <unordered_set>
#include <vector>

namespace adios2
{
namespace helper
{

const std::vector<std::vector<int>> Handshake(const std::string &filename,
                                              const char mode,
                                              const int timeoutSeconds,
                                              MPI_Comm localComm)
{
    std::vector<std::vector<int>> ret(3);

    int localRank;
    int localSize;
    int worldRank;
    int worldSize;

    MPI_Comm_rank(localComm, &localRank);
    MPI_Comm_size(localComm, &localSize);

    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    std::vector<int> allLocalRanks(localSize);

    MPI_Gather(&worldRank, 1, MPI_INT, allLocalRanks.data(), 1, MPI_INT, 0,
               localComm);

    if (localRank == 0)
    {
        std::ofstream fs;
        fs.open(filename + "." + mode);
        for (auto rank : allLocalRanks)
        {
            fs << rank << std::endl;
        }
        fs.close();

        if (mode == 'r')
        {

            for (auto i : allLocalRanks)
            {
                ret[0].push_back(i);
                ret[2].push_back(i);
            }

            std::ofstream fsc;
            fsc.open(filename + ".r.c");
            fsc << "completed";
            fsc.close();

            while (true)
            {
                std::ifstream fs;
                try
                {
                    fs.open(filename + ".w.c");
                    std::string line;
                    std::getline(fs, line);
                    if (line != "completed")
                    {
                        continue;
                    }
                    fs.close();
                    remove((filename + ".w.c\0").c_str());
                    break;
                }
                catch (...)
                {
                    continue;
                }
            }

            std::ifstream fs;
            fs.open(filename + ".w");
            for (std::string line; std::getline(fs, line);)
            {
                ret[0].push_back(std::stoi(line));
                ret[1].push_back(std::stoi(line));
            }
            fs.close();
            remove((filename + ".w\0").c_str());
        }
        else if (mode == 'w')
        {
            for (auto i : allLocalRanks)
            {
                ret[0].push_back(i);
                ret[1].push_back(i);
            }

            std::ofstream fsc;
            fsc.open(filename + ".w.c");
            fsc << "completed";
            fsc.close();

            while (true)
            {
                std::ifstream fs;
                try
                {
                    fs.open(filename + ".r.c");
                    std::string line;
                    std::getline(fs, line);
                    if (line != "completed")
                    {
                        continue;
                    }
                    fs.close();
                    remove((filename + ".r.c\0").c_str());
                    break;
                }
                catch (...)
                {
                    continue;
                }
            }

            std::ifstream fs;
            fs.open(filename + ".r");
            for (std::string line; std::getline(fs, line);)
            {
                ret[0].push_back(std::stoi(line));
                ret[2].push_back(std::stoi(line));
            }
            fs.close();
            remove((filename + ".r\0").c_str());
        }
    }

    int dims[3];

    if (localRank == 0)
    {
        for (int i = 0; i < 3; ++i)
        {
            dims[i] = ret[i].size();
            std::sort(ret[i].begin(), ret[i].end());
        }
    }

    MPI_Bcast(dims, 3, MPI_INT, 0, localComm);

    if (localRank != 0)
    {
        for (int i = 0; i < 3; ++i)
        {
            ret[i].resize(dims[i]);
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        MPI_Bcast(ret[i].data(), ret[i].size(), MPI_INT, 0, localComm);
    }

    return ret;
}

} // end namespace helper
} // end namespace adios2
