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

namespace adios2
{
namespace transportman
{

StagingMan::StagingMan(MPI_Comm mpiComm, Mode openMode, const int timeout,
                       const bool debugMode)
: m_MpiComm(mpiComm), m_DebugMode(debugMode), m_Timeout(timeout),
  m_OpenMode(openMode)
{
}

StagingMan::~StagingMan() {}

void StagingMan::OpenWriteTransport(std::string fullAddress)
{
    m_Transport = std::make_shared<transport::SocketZmqReqRep>(
        m_MpiComm, m_Timeout, m_DebugMode);
    m_Transport->Open(fullAddress, Mode::Write);
}

void StagingMan::OpenReadTransport()
{
    m_Transport = std::make_shared<transport::SocketZmqReqRep>(
        m_MpiComm, m_Timeout, m_DebugMode);
}

std::shared_ptr<std::vector<char>>
StagingMan::Request(const std::vector<char> &request,
                    const std::string &address, const size_t maxReplySize)
{
    if (maxReplySize > 0)
    {
        m_MaxReplySize = maxReplySize;
    }

    m_Transport->Open(address, m_OpenMode);
    m_Transport->Write(request.data(), request.size());
    Transport::Status status;
    auto reply = std::make_shared<std::vector<char>>();
    reply->resize(m_MaxReplySize);
    m_Transport->IRead(reply->data(), m_MaxReplySize, status);
    reply->resize(status.Bytes);
    m_Transport->Close();
    return reply;
}

void StagingMan::ReceiveRequest(std::vector<char> &request,
                                const size_t maxRequestSize)
{
    if (maxRequestSize > 0)
    {
        m_MaxRequestSize = maxRequestSize;
    }

    request.resize(m_MaxRequestSize);
    Transport::Status status;
    m_Transport->IRead(request.data(), m_MaxRequestSize, status);
    request.resize(status.Bytes);
}

void StagingMan::SendReply(std::shared_ptr<std::vector<char>> reply)
{
    m_Transport->Write(reply->data(), reply->size());
}

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
