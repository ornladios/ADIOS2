/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingMan.cpp
 *
 *  Created on: Oct 1, 2018
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include <iostream>

#include "StagingMan.h"

#ifdef ADIOS2_HAVE_ZEROMQ
#include "adios2/toolkit/transport/socket/SocketZmqP2P.h"
#include "adios2/toolkit/transport/socket/SocketZmqPubSub.h"
#endif

namespace adios2
{
namespace transportman
{

StagingMan::StagingMan(MPI_Comm mpiComm, const bool debugMode) {}

StagingMan::~StagingMan() {}

void StagingMan::SetMaxReceiveBuffer(size_t size) { m_MaxReceiveBuffer = size; }

void StagingMan::OpenTransports(const std::vector<Params> &paramsVector,
                                const Mode mode, const bool profile)
{
}

void StagingMan::Write(std::shared_ptr<std::vector<char>> buffer, size_t id) {}

void StagingMan::Write(const std::vector<char> &buffer, size_t transportId) {}

std::shared_ptr<std::vector<char>> StagingMan::Read(size_t id) {}

void StagingMan::PushBufferQueue(std::shared_ptr<std::vector<char>> v,
                                 size_t id)
{
}

std::shared_ptr<std::vector<char>> StagingMan::PopBufferQueue(size_t id) {}

void StagingMan::WriteThread(std::shared_ptr<Transport> transport, size_t id) {}

bool StagingMan::GetBoolParameter(const Params &params, const std::string key)
{
    auto itKey = params.find(key);
    if (itKey != params.end())
    {
        std::string value = itKey->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value == "yes" || value == "true")
        {
            return true;
        }
        else if (value == "no" || value == "false")
        {
            return false;
        }
    }
    return false;
}

bool StagingMan::GetStringParameter(const Params &params, const std::string key,
                                    std::string &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = it->second;
        return true;
    }
    return false;
}

bool StagingMan::GetIntParameter(const Params &params, const std::string key,
                                 int &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        try
        {
            value = std::stoi(it->second);
            return true;
        }
        catch (std::exception &e)
        {
            std::cout << "Parameter " << key
                      << " should be an integer in string format. However, "
                      << e.what() << " has been caught while trying to convert "
                                     "the value to an integer."
                      << std::endl;
            return false;
        }
    }
    return false;
}

} // end namespace transportman
} // end namespace adios2
