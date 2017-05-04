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

#include <iostream> //needs to go away, this is just for demo purposes

#include "adios2/ADIOSMPI.h"

namespace adios
{

#define H5_ERROR std::cout << "[ADIOS H5 ERROR] "

HDF5Common::HDF5Common()
: m_WriteMode(false), m_Total_timestep(0), m_CurrentTimeStep(0)
{
    DefH5T_COMPLEX_FLOAT = H5Tcreate(H5T_COMPOUND, sizeof(std::complex<float>));
    H5Tinsert(DefH5T_COMPLEX_FLOAT, "freal", 0, H5T_NATIVE_FLOAT);
    H5Tinsert(DefH5T_COMPLEX_FLOAT, "fimg", H5Tget_size(H5T_NATIVE_FLOAT),
              H5T_NATIVE_FLOAT);

    DefH5T_COMPLEX_DOUBLE =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<double>));
    H5Tinsert(DefH5T_COMPLEX_DOUBLE, "dreal", 0, H5T_NATIVE_DOUBLE);
    H5Tinsert(DefH5T_COMPLEX_DOUBLE, "dimg", H5Tget_size(H5T_NATIVE_DOUBLE),
              H5T_NATIVE_DOUBLE);

    DefH5T_COMPLEX_LongDOUBLE =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<long double>));
    H5Tinsert(DefH5T_COMPLEX_LongDOUBLE, "ldouble real", 0, H5T_NATIVE_LDOUBLE);
    H5Tinsert(DefH5T_COMPLEX_LongDOUBLE, "ldouble img",
              H5Tget_size(H5T_NATIVE_LDOUBLE), H5T_NATIVE_LDOUBLE);
}

void HDF5Common::H5_Init(const std::string name, MPI_Comm m_MPIComm,
                         bool toWrite)
{
    m_WriteMode = toWrite;

    //
    m_Plist_id = H5Pcreate(H5P_FILE_ACCESS);

#ifdef ADIOS2_HAVE_MPI
    H5Pset_fapl_mpio(m_Plist_id, m_MPIComm, MPI_INFO_NULL);
#endif

    std::string ts0 = "/TimeStep0";

    if (toWrite)
    {
        /*
         * Create a new file collectively and release property list identifier.
         */
        m_File_id =
            H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, m_Plist_id);
        if (m_File_id >= 0)
        {
            m_Group_id = H5Gcreate2(m_File_id, ts0.c_str(), H5P_DEFAULT,
                                    H5P_DEFAULT, H5P_DEFAULT);
        }
    }
    else
    {
        // read a file collectively
        m_File_id = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (m_File_id >= 0)
        {
            m_Group_id = H5Gopen(m_File_id, ts0.c_str(), H5P_DEFAULT);
        }
    }

    H5Pclose(m_Plist_id);
}

// HDF5Common::~HDF5Common() {}

void HDF5Common::WriteTimeSteps()
{
    if (m_File_id < 0)
    {
        // std::cerr<<"[ADIOS HDF5Error]: Invalid file to record timestep
        // to."<<std::endl;
        H5_ERROR << "Invalid file to record timestep to." << std::endl;
        return;
    }

    if (!m_WriteMode)
    {
        return;
    }

    hid_t s = H5Screate(H5S_SCALAR);

    hid_t attr = H5Acreate(m_File_id, "NumTimeSteps", H5T_NATIVE_UINT, s,
                           H5P_DEFAULT, H5P_DEFAULT);
    uint totalts = m_CurrentTimeStep + 1;

    if (m_Group_id < 0)
    {
        totalts = m_CurrentTimeStep;
    }

    H5Awrite(attr, H5T_NATIVE_UINT, &totalts);

    H5Sclose(s);
    H5Aclose(attr);
}

int HDF5Common::GetNumTimeSteps()
{
    if (m_WriteMode)
    {
        return -1;
    }

    if (m_File_id < 0)
    {
        std::cerr
            << "[ADIOS HDF5Error]: Invalid file to read timestep attribute."
            << std::endl;
        return -1;
    }

    if (m_Total_timestep <= 0)
    {
        hid_t attr = H5Aopen(m_File_id, "NumTimeSteps", H5P_DEFAULT);

        H5Aread(attr, H5T_NATIVE_UINT, &m_Total_timestep);
        H5Aclose(attr);
    }

    return m_Total_timestep;
}

void HDF5Common::H5_Close()
{
    WriteTimeSteps();

    if (m_Group_id >= 0)
    {
        H5Gclose(m_Group_id);
    }

    H5Fclose(m_File_id);
}

void HDF5Common::H5_Advance(int totalts)
{
    m_CurrentTimeStep++;
    if (m_CurrentTimeStep > 0)
    {
        H5Gclose(m_Group_id);
        m_Group_id = -1;
    }

    std::string tsname = "/TimeStep";
    tsname.append(std::to_string(m_CurrentTimeStep));

    if (m_WriteMode)
    {
        // m_Group_id = H5Gcreate2(m_File_id, tsname.c_str(), H5P_DEFAULT,
        //                       H5P_DEFAULT, H5P_DEFAULT);
    }
    else
    {
        if ((totalts > 0) && (totalts <= m_CurrentTimeStep))
        {
            return;
        }
        // std::cout<<" ... current  group "<<tsname.c_str()<<std::endl;
        m_Group_id = H5Gopen(m_File_id, tsname.c_str(), H5P_DEFAULT);
    }
}

void HDF5Common::CheckWriteGroup()
{
    if (!m_WriteMode)
    {
        return;
    }

    if (m_Group_id >= 0)
    {
        return;
    }
    std::string tsname = "/TimeStep";
    tsname.append(std::to_string(m_CurrentTimeStep));

    m_Group_id = H5Gcreate2(m_File_id, tsname.c_str(), H5P_DEFAULT, H5P_DEFAULT,
                            H5P_DEFAULT);
}
}
