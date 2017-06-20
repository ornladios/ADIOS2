/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosSystem.cpp implementation of adiosSystem.h functions
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "adiosSystem.h"

#include <ctime> //std::ctime

#include <chrono> //system_clock, now

#include <adios2sys/SystemTools.hxx>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosString.h"

namespace adios2
{

bool CreateDirectory(const std::string &fullPath) noexcept
{
    return adios2sys::SystemTools::MakeDirectory(fullPath);
}

bool IsLittleEndian() noexcept
{
    uint16_t hexa = 0x1234;
    return *reinterpret_cast<uint8_t *>(&hexa) != 0x12; // NOLINT
}

std::string LocalTimeDate() noexcept
{
    std::time_t now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    return std::string(std::ctime(&now));
}

std::string BroadcastString(const std::string &input, MPI_Comm mpiComm)
{
    std::string receivedInput;
    size_t characterCount = 0;

    int rank;
    MPI_Comm_rank(mpiComm, &rank);

    if (rank == 0) // sender
    {
        characterCount = input.size();

        // broadcast size for allocation
        MPI_Bcast(&characterCount, 1, ADIOS2_MPI_SIZE_T, 0, mpiComm);

        // broadcast contents
        MPI_Bcast(const_cast<char *>(input.c_str()),
                  static_cast<int>(characterCount), MPI_CHAR, 0, mpiComm);

        return input;
    }
    else // receivers
    {
        // receive size
        MPI_Bcast(&characterCount, 1, ADIOS2_MPI_SIZE_T, 0, mpiComm);

        // allocate receiver
        std::vector<char> stringReceiver(characterCount);
        MPI_Bcast(stringReceiver.data(), static_cast<int>(characterCount),
                  MPI_CHAR, 0, mpiComm);

        receivedInput.assign(stringReceiver.begin(), stringReceiver.end());
    }

    return receivedInput;
}

} // end namespace adios
