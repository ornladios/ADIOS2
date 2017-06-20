/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5WriterP.cpp
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include "HDF5WriterP.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

HDF5WriterP::HDF5WriterP(IO &io, const std::string &name,
                         const OpenMode openMode, MPI_Comm mpiComm)
: Engine("HDF5Writer", io, name, openMode, mpiComm), m_H5File(io.m_DebugMode)
{
    m_EndMessage = ", in call to IO HDF5Writer Open " + m_Name + "\n";
    Init();
}

HDF5WriterP::~HDF5WriterP() { Close(); }

// PRIVATE
void HDF5WriterP::Init()
{
    if (m_OpenMode != OpenMode::Write && m_OpenMode != OpenMode::Append)
    {
        throw std::invalid_argument(
            "ERROR: HDF5Writer only support OpenMode::Write or "
            "OpenMode::Append "
            ", in call to ADIOS Open or HDF5Writer constructor\n");
    }

    m_H5File.Init(m_Name, m_MPIComm, true);
}

#define declare_type(T)                                                        \
    void HDF5WriterP::DoWrite(Variable<T> &variable, const T *values)          \
    {                                                                          \
        DoWriteCommon(variable, values);                                       \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void HDF5WriterP::Advance(const float timeoutSeconds) { m_H5File.Advance(); }

void HDF5WriterP::Close(const int transportIndex) { m_H5File.Close(); }

template <class T>
void HDF5WriterP::DoWriteCommon(Variable<T> &variable, const T *values)
{
    variable.m_AppValues = values;
    m_WrittenVariables.insert(variable.m_Name);
    m_H5File.Write(variable, values);
}

} // end namespace adios
