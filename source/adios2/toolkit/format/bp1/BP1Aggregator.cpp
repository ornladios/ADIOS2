/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Aggregator.cpp
 *
 *  Created on: Mar 21, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP1Aggregator.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>
/// \endcond

#include <iostream>

#include "adios2/ADIOSMPI.h"

namespace adios
{
namespace format
{

BP1Aggregator::BP1Aggregator(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
}

std::string BP1Aggregator::GetGlobalProfilingLog(const std::string &rankLog)
{
    std::string profilingLog;

    if (m_RankMPI == 0)
    {
        const unsigned int sizeMPI = static_cast<const unsigned int>(m_SizeMPI);
        std::vector<std::vector<char>> rankLogs(sizeMPI - 1); // other ranks
        std::vector<int> rankLogsSizes(sizeMPI - 1, -1);      // init with -1
        std::vector<MPI_Request> requests(sizeMPI);
        std::vector<MPI_Status> statuses(sizeMPI);

        // first receive sizes
        for (unsigned int i = 1; i < sizeMPI; ++i)
        {
            MPI_Irecv(&rankLogsSizes[i - 1], 1, MPI_INT, i, 0, m_MPIComm,
                      &requests[i]);
        }

        for (unsigned int i = 1; i < sizeMPI; ++i)
        {
            MPI_Wait(&requests[i], &statuses[i]);
            if (m_DebugMode)
            {
                if (rankLogsSizes[i - 1] == -1)
                    throw std::runtime_error(
                        "ERROR: couldn't get size from rank " +
                        std::to_string(i) +
                        ", in ADIOS aggregator for Profiling.log\n");
            }
            rankLogs[i - 1].resize(rankLogsSizes[i - 1]); // allocate with zeros
        }

        // receive rankLog from other ranks
        for (unsigned int i = 1; i < sizeMPI; ++i)
        {
            MPI_Irecv(rankLogs[i - 1].data(), rankLogsSizes[i - 1], MPI_CHAR, i,
                      1, m_MPIComm, &requests[i]);
        }

        for (unsigned int i = 1; i < sizeMPI; ++i)
        {
            MPI_Wait(&requests[i], &statuses[i]);
        }

        // write global string
        // key is to reserve memory first
        profilingLog.reserve(rankLog.size() * m_SizeMPI);

        profilingLog += "{\n";
        profilingLog += rankLog + "\n";
        for (unsigned int i = 1; i < sizeMPI; ++i)
        {
            const std::string rankLogStr(rankLogs[i - 1].data(),
                                         rankLogs[i - 1].size());
            profilingLog += rankLogStr + ",";
            if (i < sizeMPI - 1)
            {
                profilingLog += "\n";
            }
        }
        profilingLog.pop_back(); // eliminate trailing comma
        profilingLog += "\n";
        profilingLog += "}\n";

        //        // write to file
        //        std::ofstream logStream(fileName);
        //        if (m_DebugMode)
        //        {
        //            if (!logStream)
        //            {
        //                throw std::ios_base::failure(
        //                    "ERROR: couldn't open profiling file " + fileName
        //                    + "\n");
        //            }
        //        }
        //
        //        logStream.write(logFile.c_str(), logFile.size());
        //        logStream.close();
    }
    else
    {
        const int rankLogSize = static_cast<const int>(rankLog.size());
        MPI_Request requestSize;
        MPI_Isend(&rankLogSize, 1, MPI_INT, 0, 0, m_MPIComm, &requestSize);

        MPI_Request requestRankLog;
        MPI_Isend(const_cast<char *>(rankLog.c_str()), rankLogSize, MPI_CHAR, 0,
                  1, m_MPIComm, &requestRankLog);
    }

    return profilingLog;
}

} // end namespace format
} // end namespace adios
