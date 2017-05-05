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
    MPI_Comm_rank(mpiComm, &_mpi_rank);
    MPI_Comm_size(mpiComm, &_mpi_size);
}

HDF5Reader::~HDF5Reader() {}

bool HDF5Reader::isValid()
{
    if (m_AccessMode != "r" && m_AccessMode != "read")
    {
        return false;
    }
    if (_H5File.m_File_id >= 0)
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

    _H5File.H5_Init(m_Name, m_MPIComm, false);
    _H5File.GetNumTimeSteps();
}

Variable<void> *HDF5Reader::InquireVariable(const std::string &variableName,
                                            const bool readIn)
{
    std::cout << "Not implemented: HDF5Reader::InquireVariable()" << std::endl;
    return NULL;
}

Variable<char> *HDF5Reader::InquireVariableChar(const std::string &variableName,
                                                const bool readIn)
{
    return NULL;
}

Variable<unsigned char> *
HDF5Reader::InquireVariableUChar(const std::string &variableName,
                                 const bool readIn)
{
    return NULL;
}

Variable<short> *
HDF5Reader::InquireVariableShort(const std::string &variableName,
                                 const bool readIn)
{
    return NULL;
}

Variable<unsigned short> *
HDF5Reader::InquireVariableUShort(const std::string &variableName,
                                  const bool readIn)
{
    return NULL;
}

Variable<int> *HDF5Reader::InquireVariableInt(const std::string &variableName,
                                              const bool)
{
    return NULL;
}

Variable<unsigned int> *
HDF5Reader::InquireVariableUInt(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<long int> *
HDF5Reader::InquireVariableLInt(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<unsigned long int> *
HDF5Reader::InquireVariableULInt(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<long long int> *
HDF5Reader::InquireVariableLLInt(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<unsigned long long int> *
HDF5Reader::InquireVariableULLInt(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<float> *
HDF5Reader::InquireVariableFloat(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<double> *
HDF5Reader::InquireVariableDouble(const std::string &variableName, const bool)
{

    if (_mpi_rank == 0)
    {
        std::cout << " ... reading var: " << variableName << std::endl;
    }
    int totalts = _H5File.GetNumTimeSteps();

    if (_mpi_rank == 0)
    {
        std::cout << " ... I saw total timesteps: " << totalts << std::endl;
    }

    while (true)
    {
        double values[1];
        UseHDFRead(variableName, values, H5T_NATIVE_DOUBLE);
        Variable<double> v =
            m_ADIOS.DefineVariable<double>("junk", adios::Dims{8});
        _H5File.ReadMe(v, values, H5T_NATIVE_DOUBLE);

        Variable<char> v1 =
            m_ADIOS.DefineVariable<char>("junk1", adios::Dims{8});

        char vv1[1];
        _H5File.ReadMe(v1, vv1, H5T_NATIVE_CHAR);

        Variable<unsigned char> v2 =
            m_ADIOS.DefineVariable<unsigned char>("junk2", adios::Dims{8});
        unsigned char vv2[1];
        _H5File.ReadMe(v2, vv2, H5T_NATIVE_UCHAR);

        Variable<unsigned int> v3 =
            m_ADIOS.DefineVariable<unsigned int>("junk3", adios::Dims{8});
        unsigned int vv3[1];
        _H5File.ReadMe(v3, vv3, H5T_NATIVE_UINT);

        Variable<int> v4 = m_ADIOS.DefineVariable<int>("junk4", adios::Dims{8});
        int vv4[1];
        _H5File.ReadMe(v4, vv4, H5T_NATIVE_INT);

        Variable<float> v5 =
            m_ADIOS.DefineVariable<float>("junk5", adios::Dims{8});
        float vv5[1];
        _H5File.ReadMe(v5, vv5, H5T_NATIVE_FLOAT);

        Variable<short> v6 =
            m_ADIOS.DefineVariable<short>("junk6", adios::Dims{8});
        short vv6[1];
        _H5File.ReadMe(v6, vv6, H5T_NATIVE_SHORT);

        Variable<unsigned short> v7 =
            m_ADIOS.DefineVariable<unsigned short>("junk7", adios::Dims{8});
        unsigned short vv7[1];
        _H5File.ReadMe(v7, vv7, H5T_NATIVE_USHORT);

        Variable<long> v8 =
            m_ADIOS.DefineVariable<long>("junk8", adios::Dims{8});
        long vv8[1];
        _H5File.ReadMe(v8, vv8, H5T_NATIVE_LONG);

        Variable<unsigned long> v9 =
            m_ADIOS.DefineVariable<unsigned long>("junk9", adios::Dims{8});
        unsigned long vv9[1];
        _H5File.ReadMe(v9, vv9, H5T_NATIVE_ULONG);

        if (_H5File.m_CurrentTimeStep >= totalts - 1)
        {
            break;
        }
        _H5File.H5_Advance(totalts);
    }
    return NULL;
}

Variable<long double> *
HDF5Reader::InquireVariableLDouble(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<std::complex<float>> *
HDF5Reader::InquireVariableCFloat(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<std::complex<double>> *
HDF5Reader::InquireVariableCDouble(const std::string &variableName, const bool)
{
    return NULL;
}

Variable<std::complex<long double>> *
HDF5Reader::InquireVariableCLDouble(const std::string &variableName, const bool)
{
    return NULL;
}

VariableCompound *
HDF5Reader::InquireVariableCompound(const std::string &variableName,
                                    const bool readIn)
{
    return NULL;
}

template <class T>
void HDF5Reader::UseHDFRead(const std::string &variableName, T *values,
                            hid_t h5type)
{
    hid_t datasetID =
        H5Dopen(_H5File.m_Group_id, variableName.c_str(), H5P_DEFAULT);
    if (_mpi_rank == 0)
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

    start[0] = _mpi_rank * dims[0] / _mpi_size;
    count[0] = dims[0] / _mpi_size;
    if (_mpi_rank == _mpi_size - 1)
    {
        count[0] = dims[0] - count[0] * (_mpi_size - 1);
        std::cout << " rank = " << _mpi_rank << ", count=" << count[0]
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
    ret = H5Dread(datasetID, h5type, mem_dataspace, filespace, H5P_DEFAULT,
                  data_array);

    for (int i = 0; i < elementsRead; i++)
    {
        std::cout << "... rank " << _mpi_rank << "   , " << data_array[i]
                  << std::endl;
    }

    H5Sclose(mem_dataspace);

    H5Sclose(filespace);
    H5Dclose(datasetID);
}

void HDF5Reader::Advance(float timeout_sec)
{
    int totalts = _H5File.GetNumTimeSteps();
    _H5File.H5_Advance(totalts);
}

void HDF5Reader::Close(const int transportIndex) { _H5File.H5_Close(); }

} // end namespace adios
