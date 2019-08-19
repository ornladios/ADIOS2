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

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosSystem.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include "adios2/toolkit/transportman/wanman/WANMan.h"

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
    int m_MpiRank;
    int m_MpiSize;
    int m_Channels;
    std::string m_WorkflowMode = "stream";
    bool m_ProvideLatest = false;
    size_t m_BufferSize = 1024 * 1024 * 1024;
    bool m_DoMonitor = false;
    int64_t m_CurrentStep = -1;

    bool m_IsLittleEndian;
    bool m_IsRowMajor;
    bool m_ContiguousMajor = true;

    int m_Verbosity = 0;

    transport::FileFStream m_FileTransport;

    std::vector<std::string> m_StreamNames;

    std::shared_ptr<transportman::WANMan> m_WANMan;
    std::shared_ptr<std::thread> m_DataThread;

    bool GetStringParameter(Params &params, std::string key,
                            std::string &value);
    bool GetBoolParameter(Params &params, std::string key, bool &value);

}; // end class DataManCommon

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANCOMMON_H_ */
