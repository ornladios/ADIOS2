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
#include "adios2/toolkit/format/bp1/BP1.h" //format::BP1Reader
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

    void Close(const int transportIndex = -1);

private:
    format::BP1Reader m_BP1BuffersReader;
    transportman::TransportMan m_FileManager;

    void Init();
    void InitTransports();
    void InitBuffers();

#define declare(T, L)                                                          \
    Variable<T> *DoInquireVariable##L(const std::string &variableName) final;

    ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare

#define declare_type(T) void DoRead(Variable<T> &variable, T *values) final;

    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    Variable<T> *InquireVariableCommon(const std::string &variableName);

    template <class T>
    void ReadCommon(Variable<T> &variable, T *values);

    // call at Init Buffers
    void ReadMinifooter();
    void ReadPGIndices();
    void ReadVariableIndices();
    void ReadAttributesIndices();
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP_BPFILEREADER_H_ */
