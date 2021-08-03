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
#if defined(ADIOS2_HAVE_DATAMAN) || defined(ADIOS2_HAVE_TABLE)

#include <iostream>
#include <thread>

#include <arpa/inet.h>  //AvailableIpAddresses() inet_ntoa
#include <net/if.h>     //AvailableIpAddresses() struct if_nameindex
#include <netinet/in.h> //AvailableIpAddresses() struct sockaddr_in
#include <string.h>     //AvailableIpAddresses() strncp
#include <sys/ioctl.h>  //AvailableIpAddresses() ioctl
#include <unistd.h>     //AvailableIpAddresses() close

#include <nlohmann_json.hpp>

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

void HandshakeWriter(Comm const &comm, size_t &appID,
                     std::vector<std::string> &fullAddresses,
                     const std::string &name, const std::string &engineName,
                     const int basePort, const int channelsPerRank,
                     const int maxRanksPerNode, const int maxAppsPerNode)
{

    int mpiRank = comm.Rank();
    int mpiSize = comm.Size();

    const std::string globalFilename = ".socket";
    const std::string globalLockFilename = ".socket.lock";

    const std::string engineFilename = name + "." + engineName;
    const std::string engineLockFilename = name + "." + engineName + ".lock";

    // Get IP address
    auto ips = helper::AvailableIpAddresses();
    std::string ip = "127.0.0.1";
    if (ips.empty() == false)
    {
        ip = ips[0];
    }

    // Check total number of writer apps
    if (mpiRank == 0)
    {
        transport::FileFStream lockCheck(comm);
        while (true)
        {
            try
            {
                lockCheck.Open(globalLockFilename, Mode::Read);
                lockCheck.Close();
            }
            catch (...)
            {
                break;
            }
        }
        transport::FileFStream lockWrite(comm);
        lockWrite.Open(globalLockFilename, Mode::Write);

        transport::FileFStream numRead(comm);
        try
        {
            numRead.Open(globalFilename, Mode::Read);
            auto size = numRead.GetSize();
            std::vector<char> numAppsChar(size);
            numRead.Read(numAppsChar.data(), numAppsChar.size());
            appID =
                1 + stoi(std::string(numAppsChar.begin(), numAppsChar.end()));
            numRead.Close();
        }
        catch (...)
        {
        }
        transport::FileFStream numWrite(comm);
        numWrite.Open(globalFilename, Mode::Write);
        std::string numAppsString = std::to_string(appID);
        numWrite.Write(numAppsString.data(), numAppsString.size());
        numWrite.Close();

        lockWrite.Close();
        remove(globalLockFilename.c_str());
    }

    appID = comm.BroadcastValue(appID);

    // Make full addresses
    for (int i = 0; i < channelsPerRank; ++i)
    {
        std::string addr =
            "tcp://" + ip + ":" +
            std::to_string(basePort + (100 * appID) +
                           (mpiRank % 1000) * channelsPerRank + i) +
            "\0";
        fullAddresses.push_back(addr);
    }
    nlohmann::json localAddressesJson = fullAddresses;
    std::string localAddressesStr = localAddressesJson.dump();
    std::vector<char> localAddressesChar(64 * channelsPerRank, '\0');
    std::memcpy(localAddressesChar.data(), localAddressesStr.c_str(),
                localAddressesStr.size());
    std::vector<char> globalAddressesChar(64 * channelsPerRank * mpiSize, '\0');
    comm.GatherArrays(localAddressesChar.data(), 64 * channelsPerRank,
                      globalAddressesChar.data());

    // Writing handshake file
    if (mpiRank == 0)
    {
        nlohmann::json globalAddressesJson;
        for (int i = 0; i < mpiSize; ++i)
        {
            auto j = nlohmann::json::parse(
                &globalAddressesChar[i * 64 * channelsPerRank]);
            for (auto &i : j)
            {
                globalAddressesJson.push_back(i);
            }
        }
        std::string globalAddressesStr = globalAddressesJson.dump();
        transport::FileFStream lockstream(comm);
        lockstream.Open(engineLockFilename, Mode::Write);
        transport::FileFStream ipstream(comm);
        ipstream.Open(engineFilename, Mode::Write);
        ipstream.Write(globalAddressesStr.data(), globalAddressesStr.size());
        ipstream.Close();
        lockstream.Close();
        remove(engineLockFilename.c_str());
    }
}

void HandshakeReader(Comm const &comm, size_t &appID,
                     std::vector<std::string> &fullAddresses,
                     const std::string &name, const std::string &engineName)
{
    const std::string engineLockFilename = name + "." + engineName + ".lock";
    const std::string engineFilename = name + "." + engineName;

    auto ips = helper::AvailableIpAddresses();
    if (ips.empty())
    {
        appID = rand();
    }
    else
    {
        std::hash<std::string> hash_fn;
        appID = hash_fn(ips[0]);
    }
    comm.BroadcastValue(appID);

    transport::FileFStream ipstream(comm);
    while (true)
    {
        try
        {
            ipstream.Open(engineFilename, Mode::Read);
            break;
        }
        catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    transport::FileFStream lockstream(comm);
    while (true)
    {
        try
        {
            lockstream.Open(engineLockFilename, Mode::Read);
            lockstream.Close();
        }
        catch (...)
        {
            break;
        }
    }

    auto size = ipstream.GetSize();
    std::vector<char> address(size);
    ipstream.Read(address.data(), size);
    ipstream.Close();
    nlohmann::json j = nlohmann::json::parse(address);
    fullAddresses = j.get<std::vector<std::string>>();
}

} // end namespace helper
} // end namespace adios2

#endif
#endif
