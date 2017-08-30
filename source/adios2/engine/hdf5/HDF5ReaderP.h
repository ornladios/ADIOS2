/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5ReaderP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_ENGINE_HDF5_HDF5READERP_H_
#define ADIOS2_ENGINE_HDF5_HDF5READERP_H_

#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/interop/hdf5/HDF5Common.h"

namespace adios2
{

class HDF5ReaderP : public Engine
{

public:
    /**
     * Constructor for single HDF5 reader engine, reads from HDF5 format
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     */
    HDF5ReaderP(IO &adios, const std::string &name, const Mode openMode,
                MPI_Comm mpiComm);

    ~HDF5ReaderP();

    bool IsValid();

    void Advance(const float timeoutSeconds = 0.0) final;

    void Close(const int transportIndex = -1) final;

    template <class T>
    void UseHDFRead(const std::string &variableName, T *values, hid_t h5Type);

private:
    interop::HDF5Common m_H5File;
    void Init() final;
};
};
#endif /* ADIOS2_ENGINE_HDF5_HDF5READERP_H_ */
