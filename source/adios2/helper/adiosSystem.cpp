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

#include <sys/stat.h>  //stat, mkdir
#include <sys/types.h> //CreateDirectory
#include <unistd.h>    //CreateDirectory

#include <chrono> //system_clock, now
#include <ctime>  //std::ctime

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosString.h"

namespace adios
{

bool CreateDirectory(const std::string &fullPath) noexcept
{
    auto lf_Mkdir = [](const std::string directory, struct stat &st) -> bool {
        if (stat(directory.c_str(), &st) == -1) // doesn't exist
        {
            mkdir(directory.c_str(), 0777);
            if (stat(directory.c_str(), &st) == -1) // doesn't exist
            {
                return false;
            }
        }
        return true;
    };

    bool directoryExists = false;
    auto directoryPosition = fullPath.find("/");

    if (fullPath[0] == '/' || fullPath[0] == '.')
    { // find the second '/'
        directoryPosition = fullPath.find("/", directoryPosition + 1);
    }

    struct stat st = {0};
    if (directoryPosition == fullPath.npos) // no subdirectories
    {
        directoryExists = lf_Mkdir(fullPath.c_str(), st);
    }
    else
    {
        std::string directory(fullPath.substr(0, directoryPosition));
        lf_Mkdir(directory.c_str(), st);

        while (directoryPosition != fullPath.npos)
        {
            directoryPosition = fullPath.find("/", directoryPosition + 1);
            directory = fullPath.substr(0, directoryPosition);
            directoryExists = lf_Mkdir(directory.c_str(), st);
        }
    }
    return directoryExists;
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

std::string BroadcastFileContents(const std::string &fileName,
                                  MPI_Comm mpiComm) noexcept
{
    std::string fileContents;
    size_t characterCount = 0;

    int rank;
    MPI_Comm_rank(mpiComm, &rank);

    if (rank == 0) // sender
    {
        fileContents = FileToString(fileName);
        characterCount = fileContents.size();

        // broadcast size for allocation
        MPI_Bcast(&characterCount, 1, ADIOS2_MPI_SIZE_T, 0, mpiComm);

        // broadcast contents
        MPI_Bcast(const_cast<char *>(fileContents.c_str()),
                  static_cast<int>(characterCount), MPI_CHAR, 0, mpiComm);
    }
    else // receivers
    {
        // receive size
        MPI_Bcast(&characterCount, 1, ADIOS2_MPI_SIZE_T, 0, mpiComm);

        // allocate receiver
        std::vector<char> fileContentsReceiver(characterCount);
        MPI_Bcast(fileContentsReceiver.data(), static_cast<int>(characterCount),
                  MPI_CHAR, 0, mpiComm);

        fileContents.assign(fileContentsReceiver.begin(),
                            fileContentsReceiver.end());
    }

    return fileContents;
}

} // end namespace adios
