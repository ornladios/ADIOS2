/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5WriterP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_ENGINE_HDF5_HDF5WRITERP_H__
#define ADIOS2_ENGINE_HDF5_HDF5WRITERP_H__

#include <hdf5.h>

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/interop/hdf5/HDF5Common.h"

namespace adios2
{

class HDF5WriterP : public Engine
{

public:
    /**
     * Constructor for HDF5 writer engine, writes in hdf5 format
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     */
    HDF5WriterP(IO &io, const std::string &name, const OpenMode openMode,
                MPI_Comm mpiComm);

    ~HDF5WriterP();

    void Advance(const float timeoutSeconds = 0.0) final;

    void Close(const int transportIndex = -1) final;

private:
    interop::HDF5Common m_H5File;

    void Init();

#define declare_type(T)                                                        \
    void DoWrite(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void DoWriteCommon(Variable<T> &variable, const T *values);
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_HDF5_HDF5WRITERP_H__ */
