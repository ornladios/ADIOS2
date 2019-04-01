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

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace interop
{

template <class T>
void HDF5Common::Write(core::Variable<T> &variable, const T *values)
{
    CheckWriteGroup();
    int dimSize = std::max(variable.m_Shape.size(), variable.m_Count.size());
    hid_t h5Type = GetHDF5Type<T>();

    if (std::is_same<T, std::string>::value)
    {
        h5Type = GetTypeStringScalar(*(std::string *)values);
    }

    if (dimSize == 0)
    {
        // write scalar
        hid_t filespaceID = H5Screate(H5S_SCALAR);
        // hid_t dsetID = CreateDataset(variable.m_Name, h5Type, filespaceID);
        std::vector<hid_t> chain;
        CreateDataset(variable.m_Name, h5Type, filespaceID, chain);
        HDF5DatasetGuard g(chain);
        hid_t dsetID = chain.back();
        /*
        hid_t dsetID =
            H5Dcreate(m_GroupId, variable.m_Name.c_str(), h5Type, filespaceID,
                      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                      */
        /*
                hid_t plistID = H5Pcreate(H5P_DATASET_XFER);
        #ifdef ADIOS2_HAVE_MPI
                H5Pset_dxpl_mpio(plistID, H5FD_MPIO_COLLECTIVE);
        #endif
        */
        if (std::is_same<T, std::string>::value)
        {
            H5Dwrite(dsetID, h5Type, H5S_ALL, H5S_ALL, m_PropertyTxfID,
                     ((std::string *)values)->data());
            H5Tclose(h5Type);
        }
        else
        {
            H5Dwrite(dsetID, h5Type, H5S_ALL, H5S_ALL, m_PropertyTxfID, values);
        }
        H5Sclose(filespaceID);
        //	CloseDataset(dsetID);

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

    std::vector<hid_t> chain;
    /*hid_t dsetID =*/CreateDataset(variable.m_Name, h5Type, fileSpace, chain);
    hid_t dsetID = chain.back();
    HDF5DatasetGuard g(chain);
    /*
    hid_t dsetID = H5Dcreate(m_GroupId, variable.m_Name.c_str(), h5Type,
                             fileSpace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                             */
    // H5Sclose(fileSpace);

    hid_t memSpace = H5Screate_simple(dimSize, count.data(), NULL);

    // Select hyperslab
    fileSpace = H5Dget_space(dsetID);
    H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset.data(), NULL,
                        count.data(), NULL);

    /*
        //  Create property list for collective dataset write.

        hid_t plistID = H5Pcreate(H5P_DATASET_XFER);
    #ifdef ADIOS2_HAVE_MPI
        H5Pset_dxpl_mpio(plistID, H5FD_MPIO_COLLECTIVE);
    #endif
    */
    herr_t status;

    status =
        H5Dwrite(dsetID, h5Type, memSpace, fileSpace, m_PropertyTxfID, values);
    if (status < 0)
    {
        if (m_DebugMode)
        {
            throw std::ios_base::failure(
                "ERROR: HDF5 file Write failed, in call to Write\n");
        }
    }

    size_t valuesSize = adios2::helper::GetTotalSize(variable.m_Count);
    T min, max;
    adios2::helper::GetMinMaxThreads(values, valuesSize, min, max, 1);

#ifdef NO_STAT
    int chainSize = chain.size();
    hid_t parentId = m_GroupId;
    if (chainSize > 1)
    {
        parentId = chain[chainSize - 2];
    }
    AddBlockInfo(variable, parentId);

    std::vector<T> stats = {min, max};
    AddStats(variable, parentId, stats);
#endif
    //    CloseDataset(dsetID);
    H5Sclose(fileSpace);
    H5Sclose(memSpace);
    //    H5Pclose(plistID);
}

template <class T>
void HDF5Common::AddStats(const core::Variable<T> &variable, hid_t parentId,
                          std::vector<T> &stats)
{

    hid_t h5Type = GetHDF5Type<T>();

    std::string statInfo_name = PREFIX_STAT + variable.m_Name;
    hsize_t numStat = stats.size(); // min, max etc
    hsize_t statDim[2] = {(hsize_t)m_CommSize, numStat};
    hid_t statSpace_id = H5Screate_simple(numStat, statDim, NULL);
    hid_t statId =
        H5Dcreate(parentId, statInfo_name.c_str(), h5Type, statSpace_id,
                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    hsize_t statLocalDim[1] = {numStat};
    hid_t statLocal_id = H5Screate_simple(1, statLocalDim, NULL);

    hsize_t statOffset[2] = {(hsize_t)m_CommRank, 0};
    hsize_t statCount[2] = {1, numStat};
    H5Sselect_hyperslab(statSpace_id, H5S_SELECT_SET, statOffset, NULL,
                        statCount, NULL);

    H5Dwrite(statId, h5Type, statLocal_id, statSpace_id, m_PropertyTxfID,
             stats.data());

    H5Sclose(statLocal_id);
    H5Sclose(statSpace_id);
    H5Dclose(statId);
    //     H5Pclose(plistID);
}

template <class T>
void HDF5Common::AddBlockInfo(const core::Variable<T> &variable, hid_t parentId)
{
    int dimSize = std::max(variable.m_Shape.size(), variable.m_Count.size());
    hsize_t metaDim[2];
    metaDim[1] = dimSize * 2;
    metaDim[0] = m_CommSize;
    hid_t metaSpace_id = H5Screate_simple(2, metaDim, NULL);
    std::string blockInfo_name = PREFIX_BLOCKINFO + variable.m_Name;
    hid_t metaId =
        H5Dcreate(parentId, blockInfo_name.c_str(), H5T_NATIVE_HSIZE,
                  metaSpace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    size_t blocks[dimSize * 2];
    for (int i = 0; i < dimSize; i++)
    {
        blocks[i + dimSize] = variable.m_Count[i];
        blocks[i] = variable.m_Start[i];
    }
    hsize_t blockDim[1] = {(hsize_t)(dimSize * 2)};
    hid_t metaLocal_id = H5Screate_simple(1, blockDim, NULL);

    hsize_t metaOffset[2] = {(hsize_t)m_CommRank, 0};
    hsize_t metaCount[2] = {1, (hsize_t)(dimSize * 2)};
    H5Sselect_hyperslab(metaSpace_id, H5S_SELECT_SET, metaOffset, NULL,
                        metaCount, NULL);

    H5Dwrite(metaId, H5T_NATIVE_HSIZE, metaLocal_id, metaSpace_id,
             m_PropertyTxfID, blocks);

    H5Sclose(metaLocal_id);
    H5Sclose(metaSpace_id);
    H5Dclose(metaId);

    //      H5Pclose(plistID);
}
//
//
template <>
hid_t HDF5Common::GetHDF5Type<std::string>()
{
    return H5T_C_S1;
}

template <>
hid_t HDF5Common::GetHDF5Type<int8_t>()
{
    return H5T_NATIVE_INT8;
}

template <>
hid_t HDF5Common::GetHDF5Type<uint8_t>()
{
    return H5T_NATIVE_UINT8;
}

template <>
hid_t HDF5Common::GetHDF5Type<int16_t>()
{
    return H5T_NATIVE_INT16;
}
template <>
hid_t HDF5Common::GetHDF5Type<uint16_t>()
{
    return H5T_NATIVE_UINT16;
}
template <>
hid_t HDF5Common::GetHDF5Type<int32_t>()
{
    return H5T_NATIVE_INT32;
}
template <>
hid_t HDF5Common::GetHDF5Type<uint32_t>()
{
    return H5T_NATIVE_UINT32;
}
template <>
hid_t HDF5Common::GetHDF5Type<int64_t>()
{
    return H5T_NATIVE_INT64;
}
template <>
hid_t HDF5Common::GetHDF5Type<uint64_t>()
{
    return H5T_NATIVE_UINT64;
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

} // end namespace interop
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_ */
