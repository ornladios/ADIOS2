/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManCommon.h
 *
 *  Created on: Feb 12, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANCOMMON_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANCOMMON_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/dataman/DataMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class DataManCommon : public Engine
{

public:
    DataManCommon(const std::string engineType, IO &io, const std::string &name,
                  const Mode mode, MPI_Comm mpiComm);

    virtual ~DataManCommon() = default;

protected:
    int m_MPIRank;
    int m_MPISize;
    int m_TransportChannels;
    std::string m_Format = "dataman";
    std::string m_WorkflowMode = "subscribe";
    size_t m_BufferSize = 1024 * 1024 * 1024;
    bool m_DoMonitor = false;
    int64_t m_CurrentStep = -1;

    bool m_IsLittleEndian;
    bool m_IsRowMajor;

    std::vector<std::string> m_StreamNames;
    std::vector<core::Operator *> m_Callbacks;

    std::shared_ptr<transportman::DataMan> m_DataMan;
    std::shared_ptr<std::thread> m_DataThread;

    virtual void IOThread(std::shared_ptr<transportman::DataMan> man) = 0;
    bool GetBoolParameter(Params &params, std::string key, bool &value);
    bool GetStringParameter(Params &params, std::string key,
                            std::string &value);
    bool GetIntParameter(Params &params, std::string key, int &value);

}; // end class DataManCommon

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANCOMMON_H_ */
