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
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosSystem.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/zmq/zmqpubsub/ZmqPubSub.h"
#include "adios2/toolkit/zmq/zmqreqrep/ZmqReqRep.h"

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
                  const Mode mode, helper::Comm comm);
    virtual ~DataManCommon();

protected:
    // external parameters
    int m_Verbosity = 0;
    size_t m_SerializerBufferSize = 128 * 1024 * 1024;
    size_t m_ReceiverBufferSize = 128 * 1024 * 1024;
    int m_Timeout = 5;
    bool m_OneToOneMode = false;

    // internal variables
    int m_MpiRank;
    int m_MpiSize;
    int64_t m_CurrentStep = -1;
    bool m_ThreadActive = true;
    bool m_IsRowMajor;
    std::string m_IPAddress;
    int m_Port = 50001;

    format::DataManSerializer m_FastSerializer;

    bool GetParameter(const Params &params, const std::string &key,
                      bool &value);
    bool GetParameter(const Params &params, const std::string &key, int &value);
    bool GetParameter(const Params &params, const std::string &key,
                      std::string &value);

}; // end class DataManCommon

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANCOMMON_H_ */
