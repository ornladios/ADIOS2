/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5ReaderP.cpp
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include "HDF5ReaderP.h"

#include "adios2/ADIOSMPI.h"

namespace adios
{

HDF5Reader::HDF5Reader(ADIOS &adios, const std::string name,
                       const std::string accessMode, MPI_Comm mpiComm,
                       const Method &method)
: Engine(adios, "HDF5Reader", name, accessMode, mpiComm, method,
         " HDF5Reader constructor (or call to ADIOS Open).\n")

{
    Init();
}

HDF5Reader::~HDF5Reader() { Close(); }

bool HDF5Reader::isValid()
{
    if (m_AccessMode != "r" && m_AccessMode != "read")
    {
        return false;
    }
    if (m_H5File.m_File_id >= 0)
    {
        return true;
    }
}
void HDF5Reader::Init()
{
    if (m_AccessMode != "r" && m_AccessMode != "read")
    {
        throw std::invalid_argument(
            "ERROR: HDF5Reader doesn't support access mode " + m_AccessMode +
            ", in call to ADIOS Open or HDF5Reader constructor\n");
    }

    m_H5File.H5_Init(m_Name, m_MPIComm, false);
    m_H5File.GetNumTimeSteps();
}

Variable<void> *HDF5Reader::InquireVariable(const std::string &variableName,
                                            const bool readIn)
{
    std::cout << "Not implemented: HDF5Reader::InquireVariable()" << std::endl;
    return nullptr;
}

Variable<char> *HDF5Reader::InquireVariableChar(const std::string &variableName,
                                                const bool readIn)
{
    return nullptr;
}

Variable<unsigned char> *
HDF5Reader::InquireVariableUChar(const std::string &variableName,
                                 const bool readIn)
{
    return nullptr;
}

Variable<short> *
HDF5Reader::InquireVariableShort(const std::string &variableName,
                                 const bool readIn)
{
    return nullptr;
}

Variable<unsigned short> *
HDF5Reader::InquireVariableUShort(const std::string &variableName,
                                  const bool readIn)
{
    return nullptr;
}

Variable<int> *HDF5Reader::InquireVariableInt(const std::string &variableName,
                                              const bool)
{
    return nullptr;
}

Variable<unsigned int> *
HDF5Reader::InquireVariableUInt(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<long int> *
HDF5Reader::InquireVariableLInt(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<unsigned long int> *
HDF5Reader::InquireVariableULInt(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<long long int> *
HDF5Reader::InquireVariableLLInt(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<unsigned long long int> *
HDF5Reader::InquireVariableULLInt(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<float> *
HDF5Reader::InquireVariableFloat(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<double> *
HDF5Reader::InquireVariableDouble(const std::string &variableName, const bool)
{

    if (m_RankMPI == 0)
    {
        std::cout << " ... reading var: " << variableName << std::endl;
    }
    int totalts = m_H5File.GetNumTimeSteps();

    if (m_RankMPI == 0)
    {
        std::cout << " ... I saw total timesteps: " << totalts << std::endl;
    }

    return nullptr;
}

Variable<long double> *
HDF5Reader::InquireVariableLDouble(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<std::complex<float>> *
HDF5Reader::InquireVariableCFloat(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<std::complex<double>> *
HDF5Reader::InquireVariableCDouble(const std::string &variableName, const bool)
{
    return nullptr;
}

Variable<std::complex<long double>> *
HDF5Reader::InquireVariableCLDouble(const std::string &variableName, const bool)
{
    return nullptr;
}

VariableCompound *
HDF5Reader::InquireVariableCompound(const std::string &variableName,
                                    const bool readIn)
{
    return nullptr;
}

template <class T>
void HDF5Reader::UseHDFRead(const std::string &variableName, T *values,
                            hid_t h5Type)
{
    hid_t datasetID =
        H5Dopen(m_H5File.m_Group_id, variableName.c_str(), H5P_DEFAULT);
    if (m_RankMPI == 0)
    {
        std::cout << " opened to read: " << variableName << std::endl;
    }

    if (datasetID < 0)
    {
        return;
    }
    hid_t filespace = H5Dget_space(datasetID);

    if (filespace < 0)
    {
        return;
    }
    int ndims = H5Sget_simple_extent_ndims(filespace);
    hsize_t dims[ndims];
    herr_t status_n = H5Sget_simple_extent_dims(filespace, dims, NULL);

    hsize_t start[ndims] = {0}, count[ndims] = {0}, stride[ndims] = {1};

    int totalElements = 1;
    for (int i = 0; i < ndims; i++)
    {
        std::cout << " [" << i << "] th dimension: " << dims[i] << std::endl;
        count[i] = dims[i];
        totalElements *= dims[i];
    }

    start[0] = m_RankMPI * dims[0] / m_SizeMPI;
    count[0] = dims[0] / m_SizeMPI;
    if (m_RankMPI == m_SizeMPI - 1)
    {
        count[0] = dims[0] - count[0] * (m_SizeMPI - 1);
        std::cout << " rank = " << m_RankMPI << ", count=" << count[0]
                  << std::endl;
    }

    hid_t ret = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, stride,
                                    count, NULL);
    if (ret < 0)
    {
        return;
    }

    hid_t mem_dataspace = H5Screate_simple(ndims, count, NULL);

    int elementsRead = 1;
    for (int i = 0; i < ndims; i++)
    {
        elementsRead *= count[i];
    }

    T data_array[elementsRead];
    ret = H5Dread(datasetID, h5Type, mem_dataspace, filespace, H5P_DEFAULT,
                  data_array);

    for (int i = 0; i < elementsRead; i++)
    {
        std::cout << "... rank " << m_RankMPI << "   , " << data_array[i]
                  << std::endl;
    }

    H5Sclose(mem_dataspace);

    H5Sclose(filespace);
    H5Dclose(datasetID);
}

void HDF5Reader::Advance(float timeoutSec)
{
    int totalts = m_H5File.GetNumTimeSteps();
    m_H5File.H5_Advance(totalts);
}

void HDF5Reader::Close(const int transportIndex) { m_H5File.H5_Close(); }

} // end namespace adios
