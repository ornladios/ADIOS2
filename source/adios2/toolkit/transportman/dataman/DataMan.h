/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_
#define ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_

#include <queue>
#include <thread>

#include "adios2/core/IO.h"
#include "adios2/core/Operator.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{
namespace transportman
{

class DataMan : public TransportMan
{

public:
    DataMan(MPI_Comm mpiComm, const bool debugMode);

    ~DataMan();

    void OpenWANTransports(const std::vector<std::string> &streamNames,
                           const Mode openMode,
                           const std::vector<Params> &parametersVector,
                           const bool profile);

    void WriteWAN(const std::vector<char> &buffer);
    void WriteWAN(std::shared_ptr<std::vector<char>> buffer);

    std::shared_ptr<std::vector<char>> ReadWAN();

    void SetMaxReceiveBuffer(size_t size);

private:
    bool m_Blocking = true;
    std::function<void(std::vector<char>)> m_Callback;

    // Objects for buffer queue
    std::queue<std::shared_ptr<std::vector<char>>> m_BufferQueue;
    void PushBufferQueue(std::shared_ptr<std::vector<char>> v);
    std::shared_ptr<std::vector<char>> PopBufferQueue();
    std::mutex m_Mutex;

    // Functions for parsing parameters
    bool GetBoolParameter(const Params &params, std::string key);
    bool GetStringParameter(const Params &params, std::string key,
                            std::string &value, std::string default_value);

    void ReadThread(std::shared_ptr<Transport> transport);
    std::vector<std::thread> m_ReadThreads;
    bool m_Reading = false;

    void WriteThread(std::shared_ptr<Transport> transport);
    std::vector<std::thread> m_WriteThreads;
    bool m_Writing = false;

    std::vector<Params> m_TransportsParameters;
    size_t m_MaxReceiveBuffer = 128 * 1024 * 1024;
    size_t m_CurrentTransport = 0;
    int m_Timeout = 5;
    const std::string m_DefaultLibrary = "zmq";
    const std::string m_DefaultIPAddress = "127.0.0.1";
    const std::string m_DefaultPort = "12306";
    const std::string m_DefaultTransportMode = "broadcast";
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_ */
