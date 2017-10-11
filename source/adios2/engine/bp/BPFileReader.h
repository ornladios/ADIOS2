/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.h
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP_BPFILEREADER_H_
#define ADIOS2_ENGINE_BP_BPFILEREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h" //format::BP1Deserializer
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{

class BPFileReader : public Engine
{

public:
    /**
     * Unique constructor
     * @param io
     * @param name
     * @param openMode only read
     * @param mpiComm
     */
    BPFileReader(IO &io, const std::string &name, const Mode openMode,
                 MPI_Comm mpiComm);

    virtual ~BPFileReader() = default;

    void PerformGets() final;

    void Close(const int transportIndex = -1);

private:
    format::BP3Deserializer m_BP3Deserializer;
    transportman::TransportMan m_FileManager;

    void Init();
    void InitTransports();
    void InitBuffer();

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    void DoGetDeferred(Variable<T> &, T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP_BPFILEREADER_H_ */
