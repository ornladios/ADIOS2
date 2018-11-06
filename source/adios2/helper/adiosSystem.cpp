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
#ifndef _WIN32
#include <arpa/inet.h> //AvailableIpAddresses() inet_ntoa
#include <net/if.h>    //AvailableIpAddresses() struct if_nameindex
#include <string.h>    //AvailableIpAddresses() strncp
#include <sys/ioctl.h> //AvailableIpAddresses() ioctl
#include <unistd.h>    //AvailableIpAddresses() close
#endif
#include <chrono> //system_clock, now
#include <ctime>
#include <iostream>  //std::cerr
#include <stdexcept> // std::runtime_error, std::exception
#include <system_error>

#include <adios2sys/SystemTools.hxx>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosString.h"

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

#ifndef _WIN32
std::vector<std::string> AvailableIpAddresses() noexcept
{
    std::vector<std::string> ips;
    int socket_handler = -1;
    struct if_nameindex *p = 0;
    struct if_nameindex *head = 0;
    if ((socket_handler = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return ips;
    }
    head = if_nameindex();
    p = if_nameindex();
    while ((p != NULL) && (p->if_name != NULL))
    {
        struct ifreq req;
        strncpy(req.ifr_name, p->if_name, IFNAMSIZ);
        if (ioctl(socket_handler, SIOCGIFADDR, &req) < 0)
        {
            if (errno == EADDRNOTAVAIL)
            {
                ++p;
                continue;
            }
            close(socket_handler);
            return ips;
        }
        const std::string ip =
            inet_ntoa(((struct sockaddr_in *)&req.ifr_addr)->sin_addr);
        if (ip != "127.0.0.1")
        {
            ips.emplace_back(ip);
        }
        ++p;
    }
    if_freenameindex(head);
    close(socket_handler);
    return ips;
}
#endif
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

} // end namespace helper
} // end namespace adios2
