/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManDeserializer.tcc Deserializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANDESERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANDESERIALIZER_TCC_

#include "DataManDeserializer.h"

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZfp.h"
#endif

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace format
{

template <class T>
int DataManDeserializer::Get(core::Variable<T> &variable, size_t step)
{

    std::shared_ptr<std::vector<DataManVar>> vec = nullptr;

    m_MutexMetaData.lock();
    const auto &i = m_MetaDataMap.find(step);
    if (i == m_MetaDataMap.end())
    {
        return -1; // step not found
    }
    else
    {
        vec = i->second;
    }
    m_MutexMetaData.unlock();

    if (vec == nullptr)
    {
        return -2; // step found but variable not found
    }
    else
    {
        for (const auto &j : *vec)
        {
            if (j.name == variable.m_Name)
            {
                if (HasOverlap(j.start, j.count, variable.m_Start,
                               variable.m_Count) == false)
                {
                    return -3; // step and variable found but variable does not
                               // have desired part
                }
                // Get the shared pointer first and then copy memory. This is
                // done in order to avoid expensive memory copy operations
                // happening inside the lock. Once the shared pointer is
                // assigned to k, its life cycle in m_BufferMap does not matter
                // any more. So even if m_BufferMap[j.index] is modified
                // somewhere else the memory
                // that this shared pointer refers to is still valid until k
                // runs out of scope.
                m_MutexBuffer.lock();
                std::shared_ptr<std::vector<char>> k = m_BufferMap[j.index];
                m_MutexBuffer.unlock();
                if (j.compression == "zfp")
                {
#ifdef ADIOS2_HAVE_ZFP
                    Params p = {{"Rate", std::to_string(j.compressionRate)}};
                    core::compress::CompressZfp zfp(p, true);
                    std::vector<char> decompressBuffer;
                    decompressBuffer.reserve(variable.PayloadSize());
                    try
                    {
                        zfp.Decompress(k->data() + j.position, j.size,
                                       decompressBuffer.data(), j.count, j.type,
                                       p);
                    }
                    catch (std::exception &e)
                    {
                        return -4; // decompression failed
                    }
                    helper::NdCopy<T>(
                        decompressBuffer.data(), j.start, j.count, true, true,
                        reinterpret_cast<char *>(variable.GetData()),
                        variable.m_Start, variable.m_Count, true, true);
#else
                    throw std::runtime_error(
                        "Data received is compressed using ZFP. However, ZFP "
                        "library is not found locally and as a result it "
                        "cannot be decompressed.");
                    return -101; // zfp library not found
#endif
                }
                else if (j.compression == "sz")
                {
#ifdef ADIOS2_HAVE_SZ
#else
                    throw std::runtime_error(
                        "Data received is compressed using SZ. However, SZ "
                        "library is not found locally and as a result it "
                        "cannot be decompressed.");
                    return -102; // sz library not found
#endif
                }
                else
                {
                    helper::NdCopy<T>(
                        k->data() + j.position, j.start, j.count, j.isRowMajor,
                        j.isLittleEndian,
                        reinterpret_cast<char *>(variable.GetData()),
                        variable.m_Start, variable.m_Count, m_IsRowMajor,
                        m_IsLittleEndian);
                }
            }
        }
    }
    return 0;
}
} // namespace format
} // namespace adios2

#endif
