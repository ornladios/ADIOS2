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
  m_Transport(timeout), m_MaxBufferSize(maxBufferSize)
{
    m_Buffer.reserve(maxBufferSize);
}

StagingMan::~StagingMan() {}

void StagingMan::OpenTransport(const std::string &fullAddress)
{
    m_Transport.Open(fullAddress, m_OpenMode);
}

void StagingMan::CloseTransport() { m_Transport.Close(); }

std::shared_ptr<std::vector<char>>
StagingMan::Request(const std::vector<char> &request,
                    const std::string &address)
{
    auto reply = std::make_shared<std::vector<char>>();

    int ret = m_Transport.Open(address, m_OpenMode);
    auto start_time = std::chrono::system_clock::now();
    while (ret)
    {
        ret = m_Transport.Open(address, m_OpenMode);
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > m_Timeout)
        {
            m_Transport.Close();
            return reply;
        }
    }

    ret = m_Transport.Write(request.data(), request.size());
    start_time = std::chrono::system_clock::now();
    while (ret < 1)
    {
        ret = m_Transport.Write(request.data(), request.size());
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > m_Timeout)
        {
            m_Transport.Close();
            return reply;
        }
    }

    ret = m_Transport.Read(m_Buffer.data(), m_MaxBufferSize);
    start_time = std::chrono::system_clock::now();
    while (ret < 1)
    {
        ret = m_Transport.Read(m_Buffer.data(), m_MaxBufferSize);
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > m_Timeout)
        {
            m_Transport.Close();
            return reply;
        }
    }

    reply->resize(ret);
    std::memcpy(reply->data(), m_Buffer.data(), ret);
    m_Transport.Close();
    return reply;
}

std::shared_ptr<std::vector<char>> StagingMan::ReceiveRequest()
{
    int bytes = m_Transport.Read(m_Buffer.data(), m_MaxBufferSize);
    if (bytes <= 0)
    {
        return nullptr;
    }
    auto request = std::make_shared<std::vector<char>>(bytes);
    std::memcpy(request->data(), m_Buffer.data(), bytes);
    return request;
}

void StagingMan::SendReply(std::shared_ptr<std::vector<char>> reply)
{
    m_Transport.Write(reply->data(), reply->size());
}

} // end namespace transportman
} // end namespace adios2
