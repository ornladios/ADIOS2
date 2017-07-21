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

#include <ctime>

#include <chrono> //system_clock, now

#include <adios2sys/SystemTools.hxx>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosString.h"

// remove ctime warning on Windows
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_DEPRECATE
#endif

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

    return std::string(ctime(&now));
}

} // end namespace adios
