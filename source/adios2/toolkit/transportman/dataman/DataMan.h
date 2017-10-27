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

#include "adios2/toolkit/transportman/TransportMan.h"
#include <json.hpp>
#include <thread>

namespace adios2
{
namespace transportman
{

class DataMan : public TransportMan
{

public:
    DataMan(MPI_Comm mpiComm, const bool debugMode);

    virtual ~DataMan() = default;

    void OpenWANTransports(const std::string &name, const OpenMode openMode,
                           const std::vector<Params> &parametersVector,
                           const bool profile);

    void WriteWAN(const void *buffer, nlohmann::json jmsg);
    void ReadWAN(void *buffer, nlohmann::json jmsg);

    void SetCallback(std::function<void(const void *, std::string, std::string,
                                        std::string, Dims)>
                         callback);

private:
    void ReadThread(std::shared_ptr<Transport> trans,
                    std::shared_ptr<Transport> ctl_trans);

    std::vector<std::shared_ptr<Transport>> m_ControlTransports;
    std::vector<std::thread> m_ControlThreads;
    size_t m_CurrentTransport = 0;
    bool m_Listening = false;

    std::function<void(const void *, std::string, std::string, std::string,
                       Dims)>
        m_CallBack = nullptr;

    nlohmann::json m_JMessage;

    /** Pick the appropriate default */
    const int m_DefaultPort = 12306;
};

} // end namespace transportman
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_ */
