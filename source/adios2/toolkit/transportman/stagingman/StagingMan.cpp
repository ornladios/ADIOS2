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

StagingMan::StagingMan(const MPI_Comm mpiComm, const Mode openMode,
                       const int timeout, const size_t maxBufferSize)
: m_MpiComm(mpiComm), m_Timeout(timeout), m_OpenMode(openMode),
  m_Transport(mpiComm, timeout), m_MaxBufferSize(maxBufferSize)
{
    m_Buffer.reserve(maxBufferSize);
}

StagingMan::~StagingMan() {}

void StagingMan::OpenTransport(const std::string fullAddress)
{
    m_Transport.Open(fullAddress, m_OpenMode);
}

void StagingMan::CloseTransport() { m_Transport.Close(); }

std::shared_ptr<std::vector<char>>
StagingMan::Request(const std::vector<char> &request,
                    const std::string &address)
{
    if (address.empty() == false)
    {
        m_Transport.Open(address, m_OpenMode);
    }
    m_Transport.Write(request.data(), request.size());
    Transport::Status status;
    m_Transport.IRead(m_Buffer.data(), m_MaxBufferSize, status);
    if (status.Bytes <= 1)
    {
        return nullptr;
    }
    auto reply = std::make_shared<std::vector<char>>(status.Bytes);
    std::memcpy(reply->data(), m_Buffer.data(), status.Bytes);
    if (address.empty() == false)
    {
        m_Transport.Close();
    }
    return reply;
}

std::shared_ptr<std::vector<char>> StagingMan::ReceiveRequest()
{
    Transport::Status status;
    m_Transport.IRead(m_Buffer.data(), m_MaxBufferSize, status);
    auto request = std::make_shared<std::vector<char>>(status.Bytes);
    std::memcpy(request->data(), m_Buffer.data(), status.Bytes);
    return request;
}

void StagingMan::SendReply(std::shared_ptr<std::vector<char>> reply)
{
    m_Transport.Write(reply->data(), reply->size());
}

} // end namespace transportman
} // end namespace adios2
