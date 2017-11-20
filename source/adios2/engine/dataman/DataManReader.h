/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.h
 *
 *  Created on: Feb 21, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_

#include <iostream> //std::cout << Needs to go

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/dataman/DataMan.h"

namespace adios2
{

class DataManReader : public Engine
{

public:
    /**
     * Constructor for dataman engine Reader for WAN communications
     * @param adios
     * @param name
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param nthreads
     */
    using json = nlohmann::json;
    DataManReader(IO &io, const std::string &name, const Mode mode,
                  MPI_Comm mpiComm);

    virtual ~DataManReader() = default;
    void Close(const int transportIndex = -1);

private:
    bool m_DoRealTime = false;
    format::BP3Deserializer m_BP3Deserializer;
    transportman::DataMan m_Man;

    void Init();
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_ */
