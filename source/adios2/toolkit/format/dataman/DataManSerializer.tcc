/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManSerializer.tcc Serializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANSERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANSERIALIZER_TCC_

#include "DataManSerializer.h"

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZfp.h"
#endif
#ifdef ADIOS2_HAVE_SZ
#include "adios2/operator/compress/CompressSZ.h"
#endif
#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZip2.h"
#endif

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace format
{

template <class T>
void DataManSerializer::PutVar(const core::Variable<T> &variable,
                               const std::string &doid, const size_t step,
                               const int rank, const std::string &address,
                               const Params &params)
{
    PutVar(variable.GetData(), variable.m_Name, variable.m_Shape, variable.m_Start,
        variable.m_Count, variable.m_MemoryStart, variable.m_MemoryCount, doid,
        step, rank, address, params);
}

template <class T>
void DataManSerializer::PutVar(const T *inputData, const std::string &varName,
                            const Dims &varShape, const Dims &varStart,
                            const Dims &varCount, const Dims &varMemStart,
                            const Dims &varMemCount, const std::string &doid,
                            const size_t step, const int rank,
                            const std::string &address, const Params &params)
{

    nlohmann::json metaj;

    metaj["N"] = varName;
    metaj["O"] = varStart;
    metaj["C"] = varCount;
    metaj["S"] = varShape;
    metaj["D"] = doid;
    metaj["M"] = m_IsRowMajor;
    metaj["E"] = m_IsLittleEndian;
    metaj["Y"] = GetType<T>();
    metaj["P"] = m_LocalBuffer->size();

    size_t datasize;
    bool compressed = false;
    const auto i = params.find("CompressionMethod");
    if (i != params.end())
    {
        std::string compressionMethod = i->second;
        std::transform(compressionMethod.begin(), compressionMethod.end(),
                       compressionMethod.begin(), ::tolower);
        if (compressionMethod == "zfp")
        {
            if (IsCompressionAvailable(compressionMethod, GetType<T>(),
                                       varCount))
            {
                compressed =
                    PutZfp<T>(metaj, datasize, inputData, varCount, params);
                if (compressed)
                {
                    metaj["Z"] = "zfp";
                }
            }
        }
        else if (compressionMethod == "sz")
        {
            if (IsCompressionAvailable(compressionMethod, GetType<T>(),
                                       varCount))
            {
                compressed =
                    PutSz<T>(metaj, datasize, inputData, varCount, params);
                if (compressed)
                {
                    metaj["Z"] = "sz";
                }
            }
        }
        else if (compressionMethod == "bzip2")
        {
            if (IsCompressionAvailable(compressionMethod, GetType<T>(),
                                       varCount))
            {
                compressed =
                    PutBZip2<T>(metaj, datasize, inputData, varCount, params);
                if (compressed)
                {
                    metaj["Z"] = "bzip2";
                }
            }
        }
        else
        {
            throw(std::invalid_argument("Compression method " + i->second +
                                        " not supported."));
        }
    }

    if (compressed == false)
    {
        datasize = std::accumulate(varCount.begin(), varCount.end(), sizeof(T),
                                   std::multiplies<size_t>());
    }
    metaj["I"] = datasize;

    if (m_LocalBuffer->capacity() < m_LocalBuffer->size() + datasize)
    {
        m_LocalBuffer->reserve((m_LocalBuffer->size() + datasize) * 2);
    }

    m_LocalBuffer->resize(m_LocalBuffer->size() + datasize);

    if (compressed)
    {
        std::memcpy(m_LocalBuffer->data() + m_LocalBuffer->size() -datasize, m_CompressBuffer.data(),
                    datasize);
    }
    else
    {
        std::memcpy(m_LocalBuffer->data() + m_LocalBuffer->size()-datasize, inputData, datasize);
    }

    m_MetadataJson[std::to_string(step)][std::to_string(rank)].emplace_back(
        metaj);
}

template <class T>
bool DataManSerializer::PutZfp(nlohmann::json &metaj, size_t &datasize,
                               const T *inputData, const Dims &varCount,
                               const Params &params)
{
#ifdef ADIOS2_HAVE_ZFP
    Params p;
    for (const auto &i : params)
    {
        std::string prefix = i.first.substr(0, 4);
        if (prefix == "zfp:" || prefix == "Zfp:" || prefix == "ZFP:")
        {
            std::string key = i.first.substr(4);
            metaj[i.first] = i.second;
            p[key] = i.second;
        }
    }
    core::compress::CompressZfp compressor(p, false);
    m_CompressBuffer.reserve(std::accumulate(varCount.begin(), varCount.end(),
                                             sizeof(T),
                                             std::multiplies<size_t>()));
    try
    {
        datasize = compressor.Compress(inputData, varCount, 4, GetType<T>(),
                                       m_CompressBuffer.data(), p);
        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "Got exception " << e.what()
                  << " from ZFP. Turned off compression." << std::endl;
    }
#else
    throw(std::invalid_argument(
        "ZFP compression used but ZFP library is not linked to ADIOS2"));
#endif
    return false;
}

template <class T>
bool DataManSerializer::PutSz(nlohmann::json &metaj, size_t &datasize,
                              const T *inputData, const Dims &varCount,
                              const Params &params)
{
#ifdef ADIOS2_HAVE_SZ
    Params p;
    for (const auto &i : params)
    {
        std::string prefix = i.first.substr(0, 3);
        if (prefix == "sz:" || prefix == "Sz:" || prefix == "SZ:")
        {
            std::string key = i.first.substr(3);
            metaj[i.first] = i.second;
            p[key] = i.second;
        }
    }
    m_CompressBuffer.reserve(std::accumulate(varCount.begin(), varCount.end(),
                                             sizeof(T),
                                             std::multiplies<size_t>()));
    core::compress::CompressSZ compressor(p, false);
    try
    {
        datasize = compressor.Compress(inputData, varCount, 4, GetType<T>(),
                                       m_CompressBuffer.data(), p);
        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "Got exception " << e.what()
                  << " from SZ. Turned off compression." << std::endl;
    }
#else
    throw(std::invalid_argument(
        "SZ compression used but SZ library is not linked to ADIOS2"));
#endif
    return false;
}

template <class T>
bool DataManSerializer::PutBZip2(nlohmann::json &metaj, size_t &datasize,
                                 const T *inputData, const Dims &varCount,
                                 const Params &params)
{
#ifdef ADIOS2_HAVE_BZIP2
    Params p;
    for (const auto &i : params)
    {
        std::string prefix = i.first.substr(0, 6);
        if (prefix == "bzip2:" || prefix == "Bzip2:" || prefix == "BZip2:" ||
            prefix == "BZIP2:")
        {
            std::string key = i.first.substr(6);
            metaj[i.first] = i.second;
            p[key] = i.second;
        }
    }
    m_CompressBuffer.reserve(std::accumulate(varCount.begin(), varCount.end(),
                                             sizeof(T),
                                             std::multiplies<size_t>()));
    core::compress::CompressBZip2 compressor(p, false);
    try
    {
        datasize = compressor.Compress(inputData, varCount, 4, GetType<T>(),
                                       m_CompressBuffer.data(), p);
        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "Got exception " << e.what()
                  << " from BZip2. Turned off compression." << std::endl;
    }
#else
    throw(std::invalid_argument(
        "BZip2 compression used but BZip2 library is not linked to ADIOS2"));
#endif
    return false;
}

template <class T>
void DataManSerializer::PutAttribute(const core::Attribute<T> &attribute,
                                     const int rank)
{
    m_MetadataJson["A"][std::to_string(rank)].emplace_back();
    auto &j = m_MetadataJson["A"][std::to_string(rank)].back();
    j["N"] = attribute.m_Name;
    j["Y"] = attribute.m_Type;
    j["V"] = attribute.m_IsSingleValue;
    if (attribute.m_IsSingleValue)
    {
        j["G"] = attribute.m_DataSingleValue;
    }
    else
    {
        j["G"] = attribute.m_DataArray;
    }
}

template <class T>
int DataManSerializer::GetVar(T *output_data, const std::string &varName,
                              const Dims &varStart, const Dims &varCount,
                              const size_t step)
{

    std::lock_guard<std::mutex> l(m_Mutex);
    std::shared_ptr<std::vector<DataManVar>> vec = nullptr;

    const auto &i = m_DataManVarMap.find(step);
    if (i == m_DataManVarMap.end())
    {
        return -1; // step not found
    }
    else
    {
        vec = i->second;
    }

    if (vec == nullptr)
    {
        return -2; // step found but variable not found
    }
    else
    {
        for (const auto &j : *vec)
        {
            if (j.name == varName)
            {
                // Get the shared pointer first and then copy memory. This is
                // done in order to avoid expensive memory copy operations
                // happening inside the lock. Once the shared pointer is
                // assigned to k, its life cycle in m_LocalBufferMap does not matter
                // any more. So even if m_LocalBufferMap[j.index] is modified
                // somewhere else the memory that this shared pointer refers to
                // is still valid until k runs out of scope.
                auto k = j.buffer;
                if (j.compression == "zfp")
                {
#ifdef ADIOS2_HAVE_ZFP
                    core::compress::CompressZfp decompressor(j.params, true);
                    std::vector<char> decompressBuffer;
                    size_t datasize =
                        std::accumulate(j.count.begin(), j.count.end(),
                                        sizeof(T), std::multiplies<size_t>());

                    decompressBuffer.reserve(datasize);
                    try
                    {
                        decompressor.Decompress(k->data() + j.position, j.size,
                                                decompressBuffer.data(),
                                                j.count, j.type, j.params);
                    }
                    catch (std::exception &e)
                    {
                        std::cout << "[DataManDeserializer::Get] Zfp "
                                     "decompression failed with exception: "
                                  << e.what() << std::endl;
                        return -4; // decompression failed
                    }
                    helper::NdCopy<T>(decompressBuffer.data(), j.start, j.count,
                                      true, true,
                                      reinterpret_cast<char *>(output_data),
                                      varStart, varCount, true, true);
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
                    core::compress::CompressSZ decompressor(j.params, true);
                    std::vector<char> decompressBuffer;
                    size_t datasize =
                        std::accumulate(j.count.begin(), j.count.end(),
                                        sizeof(T), std::multiplies<size_t>());

                    decompressBuffer.reserve(datasize);
                    try
                    {
                        decompressor.Decompress(k->data() + j.position, j.size,
                                                decompressBuffer.data(),
                                                j.count, j.type, j.params);
                    }
                    catch (std::exception &e)
                    {
                        std::cout << "[DataManDeserializer::Get] Zfp "
                                     "decompression failed with exception: "
                                  << e.what() << std::endl;
                        return -4; // decompression failed
                    }
                    helper::NdCopy<T>(decompressBuffer.data(), j.start, j.count,
                                      true, true,
                                      reinterpret_cast<char *>(output_data),
                                      varStart, varCount, true, true);
#else
                    throw std::runtime_error(
                        "Data received is compressed using SZ. However, SZ "
                        "library is not found locally and as a result it "
                        "cannot be decompressed.");
                    return -102; // sz library not found
#endif
                }
                else if (j.compression == "bzip2")
                {
#ifdef ADIOS2_HAVE_BZIP2
                    core::compress::CompressBZip2 decompressor(j.params, true);
                    std::vector<char> decompressBuffer;
                    size_t datasize =
                        std::accumulate(j.count.begin(), j.count.end(),
                                        sizeof(T), std::multiplies<size_t>());

                    decompressBuffer.reserve(datasize);
                    try
                    {
                        decompressor.Decompress(k->data() + j.position, j.size,
                                                decompressBuffer.data(),
                                                datasize);
                    }
                    catch (std::exception &e)
                    {
                        std::cout << "[DataManDeserializer::Get] Zfp "
                                     "decompression failed with exception: "
                                  << e.what() << std::endl;
                        return -4; // decompression failed
                    }
                    helper::NdCopy<T>(decompressBuffer.data(), j.start, j.count,
                                      true, true,
                                      reinterpret_cast<char *>(output_data),
                                      varStart, varCount, true, true);
#else
                    throw std::runtime_error(
                        "Data received is compressed using BZip2. However, "
                        "BZip2 library is not found locally and as a result it "
                        "cannot be decompressed.");
                    return -103; // bzip2 library not found
#endif
                }
                else
                {
                    helper::NdCopy<T>(
                        k->data() + j.position, j.start, j.count, j.isRowMajor,
                        j.isLittleEndian, reinterpret_cast<char *>(output_data),
                        varStart, varCount, m_IsRowMajor, m_IsLittleEndian);
                }
            }
        }
    }
    return 0;
}

} // namespace format
} // namespace adios2

#endif
