/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5Common.cpp
 *
 *  Created on: April 20, 2017
 *      Author: Junmin
 */

#include "HDF5Common.h"
#include "HDF5Common.tcc"

#include <complex>
#include <ios>
#include <iostream>
#include <stdexcept>

#include "adios2/ADIOSMPI.h"

namespace adios
{
namespace interop
{

HDF5Common::HDF5Common(const bool debugMode) : m_DebugMode(debugMode)
{
    m_DefH5TypeComplexFloat =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<float>));
    H5Tinsert(m_DefH5TypeComplexFloat, "freal", 0, H5T_NATIVE_FLOAT);
    H5Tinsert(m_DefH5TypeComplexFloat, "fimg", H5Tget_size(H5T_NATIVE_FLOAT),
              H5T_NATIVE_FLOAT);

    m_DefH5TypeComplexDouble =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<double>));
    H5Tinsert(m_DefH5TypeComplexDouble, "dreal", 0, H5T_NATIVE_DOUBLE);
    H5Tinsert(m_DefH5TypeComplexDouble, "dimg", H5Tget_size(H5T_NATIVE_DOUBLE),
              H5T_NATIVE_DOUBLE);

    m_DefH5TypeComplexLongDouble =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<long double>));
    H5Tinsert(m_DefH5TypeComplexLongDouble, "ldouble real", 0,
              H5T_NATIVE_LDOUBLE);
    H5Tinsert(m_DefH5TypeComplexLongDouble, "ldouble img",
              H5Tget_size(H5T_NATIVE_LDOUBLE), H5T_NATIVE_LDOUBLE);
}

void HDF5Common::Init(const std::string &name, MPI_Comm comm, bool toWrite)
{
    m_WriteMode = toWrite;
    m_PropertyListId = H5Pcreate(H5P_FILE_ACCESS);

#ifdef ADIOS2_HAVE_MPI
    H5Pset_fapl_mpio(m_PropertyListId, comm, MPI_INFO_NULL);
#endif

    std::string ts0 = "/TimeStep0";

    if (toWrite)
    {
        /*
         * Create a new file collectively and release property list identifier.
         */
        m_FileId = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
                             m_PropertyListId);
        if (m_FileId >= 0)
        {
            m_GroupId = H5Gcreate2(m_FileId, ts0.c_str(), H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT);

            if (m_DebugMode)
            {
                if (m_GroupId < 0)
                {
                    throw std::ios_base::failure(
                        "ERROR: Unable to create HDF5 group " + ts0 +
                        " in call to Open\n");
                }
            }
        }
    }
    else
    {
        // read a file collectively
        m_FileId = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (m_FileId >= 0)
        {
            m_GroupId = H5Gopen(m_FileId, ts0.c_str(), H5P_DEFAULT);
        }
    }

    H5Pclose(m_PropertyListId);
}

void HDF5Common::WriteTimeSteps()
{
    if (m_FileId < 0)
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: invalid HDF5 file to record "
                                        "timestep to, in call to Write\n");
        }
    }

    if (!m_WriteMode)
    {
        return;
    }

    hid_t s = H5Screate(H5S_SCALAR);

    hid_t attr = H5Acreate(m_FileId, "NumTimeSteps", H5T_NATIVE_UINT, s,
                           H5P_DEFAULT, H5P_DEFAULT);
    uint totalTimeSteps = m_CurrentTimeStep + 1;

    if (m_GroupId < 0)
    {
        totalTimeSteps = m_CurrentTimeStep;
    }

    H5Awrite(attr, H5T_NATIVE_UINT, &totalTimeSteps);

    H5Sclose(s);
    H5Aclose(attr);
}

unsigned int HDF5Common::GetNumTimeSteps()
{
    if (m_WriteMode)
    {
        return -1;
    }

    if (m_FileId < 0)
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: invalid HDF5 file to read timestep attribute.\n");
        }
    }

    if (m_NumTimeSteps <= 0)
    {
        hid_t attr = H5Aopen(m_FileId, "NumTimeSteps", H5P_DEFAULT);

        H5Aread(attr, H5T_NATIVE_UINT, &m_NumTimeSteps);
        H5Aclose(attr);
    }

    return m_NumTimeSteps;
}

void HDF5Common::Close()
{
    if (m_FileId < 0)
    {
        return;
    }

    WriteTimeSteps();

    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
    }

    H5Fclose(m_FileId);
    m_FileId = -1;
    m_GroupId = -1;
}

void HDF5Common::Advance()
{
    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
        m_GroupId = -1;
    }

    if (m_WriteMode)
    {
        // m_GroupId = H5Gcreate2(m_FileId, tsname.c_str(), H5P_DEFAULT,
        //                       H5P_DEFAULT, H5P_DEFAULT);
    }
    else
    {
        if (m_NumTimeSteps == 0)
        {
            GetNumTimeSteps();
        }
        if (m_CurrentTimeStep + 1 >= m_NumTimeSteps)
        {
            return;
        }

        std::string timeStepName =
            "/TimeStep" + std::to_string(m_CurrentTimeStep + 1);
        m_GroupId = H5Gopen(m_FileId, timeStepName.c_str(), H5P_DEFAULT);
        if (m_GroupId < 0)
        {
            throw std::ios_base::failure("ERROR: unable to open HDF5 group " +
                                         timeStepName + ", in call to Open\n");
        }
    }
    ++m_CurrentTimeStep;
}

void HDF5Common::CheckWriteGroup()
{
    if (!m_WriteMode)
    {
        return;
    }
    if (m_GroupId >= 0)
    {
        return;
    }

    std::string timeStepName = "/TimeStep" + std::to_string(m_CurrentTimeStep);
    m_GroupId = H5Gcreate2(m_FileId, timeStepName.c_str(), H5P_DEFAULT,
                           H5P_DEFAULT, H5P_DEFAULT);

    if (m_DebugMode)
    {
        if (m_GroupId < 0)
        {
            throw std::ios_base::failure(
                "ERROR: HDF5: Unable to create group " + timeStepName);
        }
    }
}

#define declare_template_instantiation(T)                                      \
    template void HDF5Common::Write(Variable<T> &variable, const T *value);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios
