/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ZmqPubSub.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include <chrono>
#include <cstring>
#include <iostream>

#include <zmq.h>

#include "ZmqPubSub.h"

namespace adios2
{
namespace zmq
{

ZmqPubSub::ZmqPubSub() {}

ZmqPubSub::~ZmqPubSub()
{
    auto start_time = std::chrono::system_clock::now();
    while (true)
    {
        m_BufferQueueMutex.lock();
        size_t s = m_BufferQueue.size();
        m_BufferQueueMutex.unlock();
        if (s == 0)
        {
            break;
        }
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > m_Timeout)
        {
            break;
        }
    }

    m_ThreadActive = false;
    if (m_Thread.joinable())
    {
        m_Thread.join();
    }
}

void ZmqPubSub::OpenPublisher(const std::string &address, const int timeout)
{
    m_Timeout = timeout;
    m_Thread = std::thread(&ZmqPubSub::WriterThread, this, address);
}

void ZmqPubSub::OpenSubscriber(const std::string &address, const int timeout,
                               const size_t bufferSize)
{
    m_Timeout = timeout;
    m_Thread = std::thread(&ZmqPubSub::ReaderThread, this, address, timeout,
                           bufferSize);
}

void ZmqPubSub::PushBufferQueue(std::shared_ptr<std::vector<char>> buffer)
{
    std::lock_guard<std::mutex> l(m_BufferQueueMutex);
    m_BufferQueue.push(buffer);
}

std::shared_ptr<std::vector<char>> ZmqPubSub::PopBufferQueue()
{
    std::lock_guard<std::mutex> l(m_BufferQueueMutex);
    if (m_BufferQueue.empty())
    {
        return nullptr;
    }
    else
    {
        auto ret = m_BufferQueue.front();
        m_BufferQueue.pop();
        return ret;
    }
}

void ZmqPubSub::WriterThread(const std::string &address)
{
    void *context = zmq_ctx_new();
    if (not context)
    {
        throw std::runtime_error("creating zmq context failed");
    }

    void *socket = zmq_socket(context, ZMQ_PUB);
    if (not socket)
    {
        throw std::runtime_error("creating zmq socket failed");
    }

    int error = zmq_bind(socket, address.c_str());
    if (error)
    {
        throw std::runtime_error("binding zmq socket failed");
    }

    while (m_ThreadActive)
    {
        auto buffer = PopBufferQueue();
        if (buffer != nullptr and buffer->size() > 0)
        {
            zmq_send(socket, buffer->data(), buffer->size(), ZMQ_DONTWAIT);
            if (m_Verbosity >= 5)
            {
                std::cout << "ZmqPubSub::WriterThread sent package size "
                          << buffer->size() << std::endl;
            }
        }
    }

    if (socket)
    {
        zmq_close(socket);
    }
    if (context)
    {
        zmq_ctx_destroy(context);
    }
}

void ZmqPubSub::ReaderThread(const std::string &address, const int timeout,
                             const size_t receiverBufferSize)
{
    void *context = zmq_ctx_new();
    if (not context)
    {
        throw std::runtime_error("creating zmq context failed");
    }

    void *socket = zmq_socket(context, ZMQ_SUB);
    if (not socket)
    {
        throw std::runtime_error("creating zmq socket failed");
    }

    int error = zmq_connect(socket, address.c_str());
    if (error)
    {
        throw std::runtime_error("connecting zmq socket failed");
    }

    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);

    std::vector<char> receiverBuffer(receiverBufferSize);

    while (m_ThreadActive)
    {
        int ret = zmq_recv(socket, receiverBuffer.data(), receiverBufferSize,
                           ZMQ_DONTWAIT);
        if (ret > 0)
        {
            auto buff = std::make_shared<std::vector<char>>(ret);
            std::memcpy(buff->data(), receiverBuffer.data(), ret);
            PushBufferQueue(buff);
            if (m_Verbosity >= 5)
            {
                std::cout << "ZmqPubSub::ReaderThread received package size "
                          << buff->size() << std::endl;
            }
        }
    }

    if (socket)
    {
        zmq_close(socket);
    }
    if (context)
    {
        zmq_ctx_destroy(context);
    }
}

} // end namespace zmq
} // end namespace adios2
