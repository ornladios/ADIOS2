/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.tcc  classes for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMAN_TCC_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMAN_TCC_

#include "DataMan.h"

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZfp.h"
#endif

#include <iostream>

namespace adios2
{
namespace format
{

#ifdef ADIOS2_HAVE_ZFP
template <class T>
bool DataManSerializer::PutZfp(core::Variable<T> &variable, std::string doid,
                               size_t step, int rank, const Params &params)
{
    nlohmann::json metaj;

    metaj["N"] = variable.m_Name;
    metaj["Y"] = variable.m_Type;
    metaj["S"] = variable.m_Shape;
    metaj["C"] = variable.m_Count;
    metaj["O"] = variable.m_Start;
    metaj["T"] = step;
    metaj["R"] = rank;
    metaj["D"] = doid;
    metaj["Z"] = "zfp";

    float rate = 2;
    const auto it = params.find("CompressionRate");
    if (it != params.end())
    {
        rate = stof(it->second);
    }
    metaj["ZR"] = rate;

    Params p = {{"Rate", std::to_string(rate)}};
    core::compress::CompressZfp zfp(p, true);
    m_CompressBuffer.reserve(variable.PayloadSize());
    size_t datasize;
    try
    {
        datasize = zfp.Compress(variable.GetData(), variable.m_Count, 4,
                                variable.m_Type, m_CompressBuffer.data(), p);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return PutRaw(variable, doid, step, rank, params);
    }
    metaj["I"] = datasize;
    std::string metastr = metaj.dump() + '\0';

    uint32_t metasize = metastr.size();
    size_t totalsize = sizeof(metasize) + metasize + datasize;
    if (m_Buffer->capacity() < m_Position + totalsize)
    {
        m_Buffer->reserve(m_Buffer->capacity() * 2);
    }

    m_Buffer->resize(m_Position + totalsize);

    std::memcpy(m_Buffer->data() + m_Position, &metasize, sizeof(metasize));
    m_Position += sizeof(metasize);

    std::memcpy(m_Buffer->data() + m_Position, metastr.c_str(), metasize);
    m_Position += metasize;

    std::memcpy(m_Buffer->data() + m_Position, m_CompressBuffer.data(),
                datasize);
    m_Position += datasize;
    return true;
}
#endif

template <class T>
bool DataManSerializer::PutRaw(core::Variable<T> &variable, std::string doid,
                               size_t step, int rank, const Params &params)
{
    nlohmann::json metaj;

    metaj["N"] = variable.m_Name;
    metaj["Y"] = variable.m_Type;
    metaj["S"] = variable.m_Shape;
    metaj["C"] = variable.m_Count;
    metaj["O"] = variable.m_Start;
    metaj["T"] = step;
    metaj["R"] = rank;
    metaj["D"] = doid;
    metaj["I"] = variable.PayloadSize();
    std::string metastr = metaj.dump() + '\0';

    uint32_t metasize = metastr.size();
    size_t datasize = variable.PayloadSize();
    size_t totalsize = sizeof(metasize) + metasize + datasize;
    if (m_Buffer->capacity() < m_Position + totalsize)
    {
        m_Buffer->reserve(m_Buffer->capacity() * 2);
    }

    m_Buffer->resize(m_Position + totalsize);

    std::memcpy(m_Buffer->data() + m_Position, &metasize, sizeof(metasize));
    m_Position += sizeof(metasize);

    std::memcpy(m_Buffer->data() + m_Position, metastr.c_str(), metasize);
    m_Position += metasize;

    std::memcpy(m_Buffer->data() + m_Position, variable.GetData(), datasize);
    m_Position += datasize;
    return true;
}

template <class T>
bool DataManSerializer::Put(core::Variable<T> &variable, std::string doid,
                            size_t step, int rank, const Params &params)
{
    auto it = params.find("CompressionMethod");
    if (it != params.end())
    {
#ifdef ADIOS2_HAVE_ZFP
        if (it->second == "zfp")
        {
            return PutZfp(variable, doid, step, rank, params);
        }
#endif
    }
    return PutRaw(variable, doid, step, rank, params);
}

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

                Box<Dims> srcBox(j.start,
                                 GetAbsolutePosition(j.start, j.count));
                Box<Dims> dstBox(
                    variable.m_Start,
                    GetAbsolutePosition(variable.m_Start, variable.m_Count));
                Box<Dims> overlapBox;

                if (GetOverlap(srcBox, dstBox, overlapBox) == false)
                {
                    return -3; // step and variable found but variable does not
                               // have desired part
                }

                // Get the shared pointer first and then copy memory. This is
                // done in order to avoid expensive memory copy operations
                // happening inside the lock. Once the shared pointer is
                // assigned to k, its life cycle in m_BufferMap does not matter
                // any more. So even if it is released somewhere else the memory
                // is still valid until k dies.
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
                    CopyLocalToGlobal(
                        reinterpret_cast<char *>(variable.GetData()), dstBox,
                        decompressBuffer.data(), srcBox, sizeof(T), overlapBox);
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
                    Box<Dims> srcbox(j.start, j.count);
                    Box<Dims> dstbox(variable.m_Start, variable.m_Count);
                    CopyLocalToGlobal(
                        reinterpret_cast<char *>(variable.GetData()), dstBox,
                        k->data() + j.position, srcBox, sizeof(T), overlapBox);
                }
            }
        }
    }
    return 0;
}
}
}

#endif
