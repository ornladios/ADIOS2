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

#include "adios2/ADIOSTypes.h"

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

} // end namespace adios
