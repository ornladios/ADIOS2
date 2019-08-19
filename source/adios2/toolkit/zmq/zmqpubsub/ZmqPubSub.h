/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ZmqPubSub.h
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_ZMQ_ZMQPUBSUB_H_
#define ADIOS2_TOOLKIT_ZMQ_ZMQPUBSUB_H_

#include <mutex>
#include <queue>
#include <thread>

namespace adios2
{
namespace zmq
{

class ZmqPubSub
{

public:
    ZmqPubSub();
    ~ZmqPubSub();

    void OpenPublisher(const std::string &address, const int timeout);
    void OpenSubscriber(const std::string &address, const int timeout,
                        const size_t receiveBufferSize);

    void PushBufferQueue(std::shared_ptr<std::vector<char>> buffer);
    std::shared_ptr<std::vector<char>> PopBufferQueue();

private:
    // For buffer queue
    std::queue<std::shared_ptr<std::vector<char>>> m_BufferQueue;
    std::mutex m_BufferQueueMutex;

    // For threads
    void WriterThread(const std::string &address);
    void ReaderThread(const std::string &address, const int timeout,
                      const size_t receiveBufferSize);
    std::thread m_Thread;
    bool m_ThreadActive = true;

    // parameters
    int m_Timeout = 10;
    int m_Verbosity = 0;
};

} // end namespace zmq
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_ZMQ_ZMQPUBSUB_H_ */
