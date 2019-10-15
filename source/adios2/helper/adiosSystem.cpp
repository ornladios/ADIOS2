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
#include <chrono> //system_clock, now
#include <ctime>
#include <iostream>  //std::cerr
#include <stdexcept> // std::runtime_error, std::exception
#include <system_error>

#include <adios2sys/SystemTools.hxx>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosString.h"

// needed by IsHDF5File()
#include "adios2/core/IO.h"
#include "adios2/toolkit/transportman/TransportMan.h"
#include <cstring>

// remove ctime warning on Windows
#ifdef _WIN32
#pragma warning(disable : 4996) // ctime warning
#endif

namespace adios2
{
namespace helper
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

bool IsRowMajor(const std::string hostLanguage) noexcept
{
    bool isRowMajor = true;

    if (hostLanguage == "Fortran" || hostLanguage == "R" ||
        hostLanguage == "Matlab")
    {
        isRowMajor = false;
    }

    return isRowMajor;
}

bool IsZeroIndexed(const std::string hostLanguage) noexcept
{
    bool isZeroIndexed = true;

    if (hostLanguage == "Fortran" || hostLanguage == "R")
    {
        isZeroIndexed = false;
    }
    return isZeroIndexed;
}

int ExceptionToError(const std::string &function)
{
    try
    {
        throw;
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << e.what() << "\n";
        std::cerr << function << "\n";
        return 1;
    }
    catch (std::system_error &e)
    {
        std::cerr << e.what() << "\n";
        std::cerr << function << "\n";
        return 2;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << e.what() << "\n";
        std::cerr << function << "\n";
        return 3;
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << "\n";
        std::cerr << function << "\n";
        return 4;
    }
}

bool IsHDF5File(const std::string &name, helper::Comm &comm,
                const std::vector<Params> &transportsParameters) noexcept
{
    bool isHDF5 = false;
    if (!comm.Rank())
    {
        try
        {
            transportman::TransportMan tm(Comm(), false);
            if (transportsParameters.empty())
            {
                std::vector<Params> defaultTransportParameters(1);
                defaultTransportParameters[0]["transport"] = "File";
                tm.OpenFiles({name}, adios2::Mode::Read,
                             defaultTransportParameters, false);
            }
            else
            {
                tm.OpenFiles({name}, adios2::Mode::Read, transportsParameters,
                             false);
            }
            const unsigned char HDF5Header[8] = {137, 72, 68, 70,
                                                 13,  10, 26, 10};
            if (tm.GetFileSize(0) >= 8)
            {
                char header[8];
                tm.ReadFile(header, 8, 0);
                tm.CloseFiles();
                isHDF5 = !std::memcmp(header, HDF5Header, 8);
            }
        }
        catch (std::ios_base::failure &)
        {
            isHDF5 = false;
        }
    }
    size_t flag = (isHDF5 ? 1 : 0);
    flag = comm.BroadcastValue(flag);
    return (flag == 1);
}

} // end namespace helper
} // end namespace adios2
