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

#include <thread>

#include "adios2/core/IO.h"
#include "adios2/core/Operator.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/TransportMan.h"

#include <json.hpp>

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

    void WriteWAN(const void *buffer, nlohmann::json jmsg);

    /**
     * For BP Format
     * @param buffer
     * @param size can't use const due to C libraries...
     */
    void WriteWAN(const void *buffer, size_t size);

    void ReadWAN(void *buffer, size_t &size);

    /**
     * Set BP3 deserializer private pointer m_BP3Deserializer, from Engine
     * @param bp3Deserializer comes from engine
     */
    void SetBP3Deserializer(format::BP3Deserializer &bp3Deserializer);
    void SetIO(IO &io);

    void SetCallback(adios2::Operator &callback);

private:
    format::BP3Deserializer *m_BP3Deserializer = nullptr;
    IO *m_IO = nullptr;
    Operator *m_Callback = nullptr;
    void ReadThread(std::shared_ptr<Transport> trans,
                    std::shared_ptr<Transport> ctl_trans,
                    const std::string stream_name, const Params trans_params);

    void RunCallback(void *buffer, std::string doid, std::string var,
                     std::string dtype, std::vector<size_t> shape);

    std::vector<std::shared_ptr<Transport>> m_ControlTransports;
    std::vector<std::thread> m_ControlThreads;
    std::vector<Params> m_TransportsParameters;

    size_t m_CurrentTransport = 0;
    bool m_Listening = false;
    nlohmann::json m_JMessage;
    size_t m_BufferSize = 1024 * 1024 * 1024;

    /** Pick the appropriate default */
    const int m_DefaultPort = 12306;
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_ */
