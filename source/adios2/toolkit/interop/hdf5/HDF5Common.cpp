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

namespace adios2
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

    // std::string ts0 = "/AdiosStep0";
    std::string ts0;
    StaticGetAdiosStepString(ts0, 0);

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

void HDF5Common::WriteAdiosSteps()
{
    if (m_FileId < 0)
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: invalid HDF5 file to record "
                                        "steps, in call to Write\n");
        }
    }

    if (!m_WriteMode)
    {
        return;
    }

    hid_t s = H5Screate(H5S_SCALAR);

    hid_t attr = H5Acreate(m_FileId, "NumSteps", H5T_NATIVE_UINT, s,
                           H5P_DEFAULT, H5P_DEFAULT);
    uint totalAdiosSteps = m_CurrentAdiosStep + 1;

    if (m_GroupId < 0)
    {
        totalAdiosSteps = m_CurrentAdiosStep;
    }

    H5Awrite(attr, H5T_NATIVE_UINT, &totalAdiosSteps);

    H5Sclose(s);
    H5Aclose(attr);
}

unsigned int HDF5Common::GetNumAdiosSteps()
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
                "ERROR: invalid HDF5 file to read step attribute.\n");
        }
    }

    if (m_NumAdiosSteps <= 0)
    {
        hid_t attr = H5Aopen(m_FileId, "NumSteps", H5P_DEFAULT);

        H5Aread(attr, H5T_NATIVE_UINT, &m_NumAdiosSteps);
        H5Aclose(attr);
    }

    return m_NumAdiosSteps;
}

// read from all time steps
void HDF5Common::ReadAllVariables(IO &io)
{
    int i = 0;
    //std::string timestepStr;
    hsize_t numObj;
    for (i = 0; i < m_NumAdiosSteps; i++)
    {
        ReadVariables(i, io);
    }
}

// read variables from the input timestep
void HDF5Common::ReadVariables(unsigned int ts, IO &io)
{
    int i = 0;
    std::string stepStr;
    hsize_t numObj;

    StaticGetAdiosStepString(stepStr, ts);
    hid_t gid = H5Gopen2(m_FileId, stepStr.c_str(), H5P_DEFAULT);
    HDF5TypeGuard g(gid, E_H5_GROUP);
    ///    if (gid > 0) {
    herr_t ret = H5Gget_num_objs(gid, &numObj);
    if (ret >= 0)
    {
        int k = 0;
        char name[50];
        for (k = 0; k < numObj; k++)
        {
            ret = H5Gget_objname_by_idx(gid, (hsize_t)k, name, sizeof(name));
            if (ret >= 0)
            {
                hid_t datasetId = H5Dopen(gid, name, H5P_DEFAULT);

                HDF5TypeGuard d(datasetId, E_H5_DATASET);
                CreateVar(io, datasetId, name);
            }
        }
    }
    /// H5Gclose(gid);
    ///}
}

template <class T>
void HDF5Common::AddVar(IO &io, std::string const &name, hid_t datasetId)
{
    Variable<T> *v = io.InquireVariable<T>(name);
    if (NULL == v)
    {
        hid_t dspace = H5Dget_space(datasetId);
        const int ndims = H5Sget_simple_extent_ndims(dspace);
        hsize_t dims[ndims];
        H5Sget_simple_extent_dims(dspace, dims, NULL);
        H5Sclose(dspace);

        Dims shape;
        shape.resize(ndims);
        if (ndims > 0)
        {
            // std::cout<<" ==> variable "<<name<<" is "<<ndims<<"D,
            // "<<dims[0]<<", "<<dims[1]<<std::endl;
            for (int i = 0; i < ndims; i++)
                shape[i] = dims[i];
        }

        auto &foo = io.DefineVariable<T>(name, shape);
        // default was set to 0 while m_AvailabelStepsStart is 1.
        // correcting
        if (0 == foo.m_AvailableStepsCount)
        {
            foo.m_AvailableStepsCount++;
        }
    }
    else
    {
        /*    if (0 == v->m_AvailableStepsCount) { // default was set to 0 while
        m_AvailabelStepsStart is 1. v->m_AvailableStepsCount ++;
        }
        */
        v->m_AvailableStepsCount++;
    }
}

void HDF5Common::CreateVar(IO &io, hid_t datasetId, std::string const &name)
{
    hid_t h5Type = H5Dget_type(datasetId);
    HDF5TypeGuard t(h5Type, E_H5_DATATYPE);

    if (H5Tequal(H5T_NATIVE_CHAR, h5Type))
    {
        AddVar<char>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_SCHAR, h5Type))
    {
        AddVar<signed char>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_UCHAR, h5Type))
    {
        AddVar<unsigned char>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_SHORT, h5Type))
    {
        AddVar<short>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_USHORT, h5Type))
    {
        AddVar<unsigned short>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_INT, h5Type))
    {
        AddVar<int>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_UINT, h5Type))
    {
        AddVar<unsigned int>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_LONG, h5Type))
    {
        AddVar<long>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_ULONG, h5Type))
    {
        AddVar<unsigned long>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_LLONG, h5Type))
    {
        AddVar<long long>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_ULLONG, h5Type))
    {
        AddVar<unsigned long long>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_FLOAT, h5Type))
    {
        AddVar<float>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_DOUBLE, h5Type))
    {
        AddVar<double>(io, name, datasetId);
    }
    else if (H5Tequal(H5T_NATIVE_LDOUBLE, h5Type))
    {
        AddVar<long double>(io, name, datasetId);
    }
    else if (H5Tequal(m_DefH5TypeComplexFloat, h5Type))
    {
        AddVar<std::complex<float>>(io, name, datasetId);
    }
    else if (H5Tequal(m_DefH5TypeComplexDouble, h5Type))
    {
        AddVar<std::complex<double>>(io, name, datasetId);
    }
    else if (H5Tequal(m_DefH5TypeComplexLongDouble, h5Type))
    {
        AddVar<std::complex<long double>>(io, name, datasetId);
    }

    // H5Tclose(h5Type);
}

void HDF5Common::Close()
{
    if (m_FileId < 0)
    {
        return;
    }

    WriteAdiosSteps();

    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
    }

    H5Fclose(m_FileId);
    m_FileId = -1;
    m_GroupId = -1;
}

void HDF5Common::SetAdiosStep(int step)
{
    if (m_WriteMode)
        throw std::ios_base::failure(
            "ERROR: unable to change step at Write MODE.");

    if (step < 0)
        throw std::ios_base::failure(
            "ERROR: unable to change to negative step.");

    GetNumAdiosSteps();

    if (step >= m_NumAdiosSteps)
        throw std::ios_base::failure(
            "ERROR: given time step is more than actual known steps.");

    if (m_CurrentAdiosStep == step)
    {
        return;
    }

    std::string stepName;
    StaticGetAdiosStepString(stepName, step);
    m_GroupId = H5Gopen(m_FileId, stepName.c_str(), H5P_DEFAULT);
    if (m_GroupId < 0)
    {
        throw std::ios_base::failure("ERROR: unable to open HDF5 group " +
                                     stepName + ", in call to Open\n");
    }

    m_CurrentAdiosStep = step;
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
        if (m_NumAdiosSteps == 0)
        {
            GetNumAdiosSteps();
        }
        if (m_CurrentAdiosStep + 1 >= m_NumAdiosSteps)
        {
            return;
        }

        // std::string stepName =
        //    "/AdiosStep" + std::to_string(m_CurrentAdiosStep + 1);
        std::string stepName;
        StaticGetAdiosStepString(stepName, m_CurrentAdiosStep + 1);
        m_GroupId = H5Gopen(m_FileId, stepName.c_str(), H5P_DEFAULT);
        if (m_GroupId < 0)
        {
            throw std::ios_base::failure("ERROR: unable to open HDF5 group " +
                                         stepName + ", in call to Open\n");
        }
    }
    ++m_CurrentAdiosStep;
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

    // std::string stepName = "/AdiosStep" +
    // std::to_string(m_CurrentAdiosStep);
    std::string stepName;
    StaticGetAdiosStepString(stepName, m_CurrentAdiosStep);
    m_GroupId = H5Gcreate2(m_FileId, stepName.c_str(), H5P_DEFAULT,
                           H5P_DEFAULT, H5P_DEFAULT);

    if (m_DebugMode)
    {
        if (m_GroupId < 0)
        {
            throw std::ios_base::failure(
                "ERROR: HDF5: Unable to create group " + stepName);
        }
    }
}

void HDF5Common::StaticGetAdiosStepString(std::string &stepName, int ts)
{
    stepName = "/Step" + std::to_string(ts);
}

#define declare_template_instantiation(T)                                      \
    template void HDF5Common::Write(Variable<T> &variable, const T *value);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios
