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
                           const std::vector<Params> &params,
                           const bool profile);

    void WriteWAN(const std::vector<char> &buffer, size_t size);

    std::shared_ptr<std::vector<char>> ReadWAN();

    void SetBP3Deserializer(format::BP3Deserializer &bp3Deserializer);
    void SetIO(IO &io);

    void SetCallback(std::function<void(std::vector<char>)> callback);
    void SetMaxReceiveBuffer(size_t size);

private:
    std::function<void(std::vector<char>)> m_Callback;
    void ReadThread(std::shared_ptr<Transport> trans);

    std::queue<std::shared_ptr<std::vector<char>>> m_BufferQueue;
    void PushBufferQueue(std::shared_ptr<std::vector<char>> v);
    std::shared_ptr<std::vector<char>> PopBufferQueue();
    std::mutex m_Mutex;

    bool GetBoolParameter(const Params &params, std::string key);

    std::vector<std::thread> m_ReadThreads;
    std::vector<Params> m_TransportsParameters;

    size_t m_MaxReceiveBuffer = 128 * 1024 * 1024;

    size_t m_CurrentTransport = 0;
    bool m_Listening = false;
    const int m_DefaultPort = 12306;
    int m_Timeout = 5;
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_ */
