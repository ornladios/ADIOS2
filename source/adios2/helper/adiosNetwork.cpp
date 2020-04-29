/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosNetwork.cpp implementation of adiosNetwork.h functions
 *
 *  Created on: March 22, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosNetwork.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/transport/file/FileFStream.h"

#ifndef _WIN32
#if defined(ADIOS2_HAVE_DATAMAN) || defined(ADIOS2_HAVE_SSC)

#include <iostream>
#include <thread>

#include <arpa/inet.h> //AvailableIpAddresses() inet_ntoa
#include <net/if.h>    //AvailableIpAddresses() struct if_nameindex
#include <string.h>    //AvailableIpAddresses() strncp
#include <sys/ioctl.h> //AvailableIpAddresses() ioctl
#include <unistd.h>    //AvailableIpAddresses() close

#include <nlohmann/json.hpp>

namespace adios2
{
namespace helper
{

#if defined(__clang__)
#if __has_feature(memory_sanitizer)
// Memory Sanitizer fails to recognize that if_nameindex initializes
// the memory in the array behind the pointer it returns.
__attribute__((no_sanitize("memory")))
#endif
#endif
std::vector<std::string>
AvailableIpAddresses() noexcept
{
    std::vector<std::string> ips;
    int socket_handler = -1;
    if ((socket_handler = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return ips;
    }
    struct if_nameindex *head = if_nameindex();
    if (!head)
    {
        close(socket_handler);
        return ips;
    }
    for (struct if_nameindex *p = head;
         !(p->if_index == 0 && p->if_name == NULL); ++p)
    {
        struct ifreq req;
        strncpy(req.ifr_name, p->if_name, IFNAMSIZ);
        if (ioctl(socket_handler, SIOCGIFADDR, &req) < 0)
        {
            if (errno == EADDRNOTAVAIL)
            {
                continue;
            }
            if_freenameindex(head);
            close(socket_handler);
            return ips;
        }
        const std::string ip =
            inet_ntoa(((struct sockaddr_in *)&req.ifr_addr)->sin_addr);
        if (ip != "127.0.0.1")
        {
            ips.emplace_back(ip);
        }
    }
    if_freenameindex(head);
    close(socket_handler);
    return ips;
}

} // end namespace helper
} // end namespace adios2

#endif
#endif
