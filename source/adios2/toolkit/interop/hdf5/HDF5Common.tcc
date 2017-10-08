/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5Common.tcc
 *
 *  Created on: Jun 1, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_TCC_
#define ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_TCC_

#include "HDF5Common.h"
#include <iostream>
#include <vector>

namespace adios2
{
namespace interop
{

template <class T>
void HDF5Common::Write(Variable<T> &variable, const T *values)
{
    CheckWriteGroup();
    int dimSize = std::max(variable.m_Shape.size(), variable.m_Count.size());
    hid_t h5Type = GetHDF5Type<T>();

    if (dimSize == 0) {
       // write scalar
       hid_t filespaceID = H5Screate(H5S_SCALAR);
       hid_t dsetID = H5Dcreate(m_GroupId, variable.m_Name.c_str(), h5Type, filespaceID,
       	                           H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
       hid_t plistID = H5Pcreate(H5P_DATASET_XFER);
       H5Pset_dxpl_mpio(plistID, H5FD_MPIO_COLLECTIVE);

       herr_t status = H5Dwrite(dsetID, h5Type, H5S_ALL, H5S_ALL, plistID, values);

       H5Sclose(filespaceID);
       H5Dclose(dsetID);

       return;
    }   
     
    std::vector<hsize_t> dimsf, count, offset;

    for (int i = 0; i < dimSize; ++i)
    {
        if (variable.m_Shape.size() == dimSize)
        {
            dimsf.push_back(variable.m_Shape[i]);
        }
        else
        {
            dimsf.push_back(variable.m_Count[i]);
        }

        if (variable.m_Count.size() == dimSize)
        {
            count.push_back(variable.m_Count[i]);
            if (variable.m_Start.size() == dimSize)
            {
                offset.push_back(variable.m_Start[i]);
            }
            else
            {
                offset.push_back(0);
            }
        }
        else
        {
            count.push_back(variable.m_Shape[i]);
            offset.push_back(0);
        }
    }
   
   
    hid_t fileSpace = H5Screate_simple(dimSize, dimsf.data(), NULL);

    hid_t dsetID = H5Dcreate(m_GroupId, variable.m_Name.c_str(), h5Type,
                             fileSpace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // H5Sclose(fileSpace);

    hid_t memSpace = H5Screate_simple(dimSize, count.data(), NULL);

    // Select hyperslab
    fileSpace = H5Dget_space(dsetID);
    H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset.data(), NULL,
                        count.data(), NULL);

    //  Create property list for collective dataset write.

    hid_t plistID = H5Pcreate(H5P_DATASET_XFER);
#ifdef ADIOS2_HAVE_MPI
    H5Pset_dxpl_mpio(plistID, H5FD_MPIO_COLLECTIVE);
#endif
    herr_t status;

    status = H5Dwrite(dsetID, h5Type, memSpace, fileSpace, plistID, values);

    if (status < 0)
    {
        if (m_DebugMode)
        {
            throw std::ios_base::failure(
                "ERROR: HDF5 file Write failed, in call to Write\n");
        }
    }

    H5Dclose(dsetID);
    H5Sclose(fileSpace);
    H5Sclose(memSpace);
    H5Pclose(plistID);
}

template <>
hid_t HDF5Common::GetHDF5Type<char>()
{
    return H5T_NATIVE_CHAR;
}
template <>
hid_t HDF5Common::GetHDF5Type<signed char>()
{
    return H5T_NATIVE_SCHAR;
}
template <>
hid_t HDF5Common::GetHDF5Type<unsigned char>()
{
    return H5T_NATIVE_UCHAR;
}
template <>
hid_t HDF5Common::GetHDF5Type<short>()
{
    return H5T_NATIVE_SHORT;
}
template <>
hid_t HDF5Common::GetHDF5Type<unsigned short>()
{
    return H5T_NATIVE_USHORT;
}
template <>
hid_t HDF5Common::GetHDF5Type<int>()
{
    return H5T_NATIVE_INT;
}
template <>
hid_t HDF5Common::GetHDF5Type<unsigned int>()
{
    return H5T_NATIVE_UINT;
}
template <>
hid_t HDF5Common::GetHDF5Type<long int>()
{
    return H5T_NATIVE_LONG;
}
template <>
hid_t HDF5Common::GetHDF5Type<unsigned long int>()
{
    return H5T_NATIVE_ULONG;
}
template <>
hid_t HDF5Common::GetHDF5Type<long long int>()
{
    return H5T_NATIVE_LLONG;
}
template <>
hid_t HDF5Common::GetHDF5Type<unsigned long long int>()
{
    return H5T_NATIVE_ULLONG;
}
template <>
hid_t HDF5Common::GetHDF5Type<float>()
{
    return H5T_NATIVE_FLOAT;
}
template <>
hid_t HDF5Common::GetHDF5Type<double>()
{
    return H5T_NATIVE_DOUBLE;
}
template <>
hid_t HDF5Common::GetHDF5Type<long double>()
{
    return H5T_NATIVE_LDOUBLE;
}
template <>
hid_t HDF5Common::GetHDF5Type<std::complex<float>>()
{
    return m_DefH5TypeComplexFloat;
}
template <>
hid_t HDF5Common::GetHDF5Type<std::complex<double>>()
{
    return m_DefH5TypeComplexDouble;
}
template <>
hid_t HDF5Common::GetHDF5Type<std::complex<long double>>()
{
    return m_DefH5TypeComplexLongDouble;
}

} // end namespace interop
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_ */
