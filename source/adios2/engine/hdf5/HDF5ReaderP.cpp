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

namespace adios2
{

HDF5ReaderP::HDF5ReaderP(IO &io, const std::string &name,
                         const Mode openMode, MPI_Comm mpiComm)
: Engine("HDF5Reader", io, name, openMode, mpiComm), m_H5File(io.m_DebugMode)
{
    m_EndMessage = ", in call to IO HDF5Reader Open " + m_Name + "\n";
    Init();
}

HDF5ReaderP::~HDF5ReaderP() { Close(); }

bool HDF5ReaderP::IsValid()
{
    bool isValid = false;

    if (m_OpenMode != Mode::Read)
    {
        return isValid;
    }
    if (m_H5File.m_FileId >= 0)
    {
        isValid = true;
    }
    return isValid;
}
void HDF5ReaderP::Init()
{
    if (m_OpenMode != Mode::Read)
    {
        throw std::invalid_argument(
            "ERROR: HDF5Reader only supports OpenMode::Read "
            ", in call to Open\n");
    }

    m_H5File.Init(m_Name, m_MPIComm, false);
    m_H5File.GetNumTimeSteps();
}

template <class T>
void HDF5ReaderP::UseHDFRead(const std::string &variableName, T *values,
                             hid_t h5Type)
{
    int rank, size;
    MPI_Comm_rank(m_MPIComm, &rank);
    MPI_Comm_size(m_MPIComm, &size);

    hid_t dataSetId =
        H5Dopen(m_H5File.m_GroupId, variableName.c_str(), H5P_DEFAULT);

    if (dataSetId < 0)
    {
        return;
    }
    hid_t fileSpace = H5Dget_space(dataSetId);

    if (fileSpace < 0)
    {
        return;
    }
    int ndims = H5Sget_simple_extent_ndims(fileSpace);
    hsize_t dims[ndims];
    herr_t status_n = H5Sget_simple_extent_dims(fileSpace, dims, NULL);

    // hsize_t start[ndims] = {0}, count[ndims] = {0}, stride[ndims] = {1};
    hsize_t start[ndims], count[ndims], stride[ndims];

    int totalElements = 1;
    for (int i = 0; i < ndims; i++)
    {
        count[i] = dims[i];
        totalElements *= dims[i];
        start[i] = 0;
        count[i] = 0;
        stride[i] = 1;
    }

    start[0] = rank * dims[0] / size;
    count[0] = dims[0] / size;
    if (rank == size - 1)
    {
        count[0] = dims[0] - count[0] * (size - 1);
    }

    hid_t ret = H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, stride,
                                    count, NULL);
    if (ret < 0)
    {
        return;
    }

    hid_t memDataSpace = H5Screate_simple(ndims, count, NULL);

    int elementsRead = 1;
    for (int i = 0; i < ndims; i++)
    {
        elementsRead *= count[i];
    }

    T data_array[elementsRead];
    ret = H5Dread(dataSetId, h5Type, memDataSpace, fileSpace, H5P_DEFAULT,
                  data_array);

    H5Sclose(memDataSpace);

    H5Sclose(fileSpace);
    H5Dclose(dataSetId);
}

void HDF5ReaderP::Advance(const float timeoutSeconds) { m_H5File.Advance(); }

void HDF5ReaderP::Close(const int transportIndex) { m_H5File.Close(); }

} // end namespace adios
