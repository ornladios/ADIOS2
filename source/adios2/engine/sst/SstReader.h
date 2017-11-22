/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstReader.h
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#ifndef ADIOS2_ENGINE_SST_SSTREADER_H_
#define ADIOS2_ENGINE_SST_SSTREADER_H_

#include <iostream> //std::cout << Needs to go

#include <mpi.h>

#include "adios2/toolkit/sst/sst.h"

#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"

namespace adios2
{

class SstReader : public Engine
{

public:
    /**
     * Constructor for sst engine Reader
     * @param adios
     * @param name
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param nthreads
     */
    SstReader(IO &io, const std::string &name, const Mode mode,
              MPI_Comm mpiComm);

    virtual ~SstReader() = default;

    void Close(const int transportIndex = -1);

private:
    void Init();
    SstStream m_Input;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_SST_SSTREADER_H_ */
