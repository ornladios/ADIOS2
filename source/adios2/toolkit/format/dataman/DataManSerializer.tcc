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
void DataManSerializer::Put(const core::Variable<T> &variable,
                            const std::string &doid, const size_t step,
                            const int rank, const Params &params,
                            const bool optimizeMetadata)
{
    Put(variable.GetData(), variable.m_Name, variable.m_Shape, variable.m_Start,
        variable.m_Count, doid, step, rank, params, optimizeMetadata);
}

template <class T>
void DataManSerializer::Put(const T *inputData, const std::string &varName,
                            const Dims &varShape, const Dims &varStart,
                            const Dims &varCount, const std::string &doid,
                            const size_t step, const int rank,
                            const Params &params, const bool optimizeMetadata)
{

    bool compressed = false;

    nlohmann::json metaj;

    // compulsory properties
    metaj["N"] = varName;
    metaj["O"] = varStart;
    metaj["C"] = varCount;
    metaj["T"] = step;

    // optional properties
    auto it = m_VarDefaultsMap.find(varName);
    if (it != m_VarDefaultsMap.end() && optimizeMetadata)
    {
        if (doid != it->second.doid)
        {
            metaj["D"] = doid;
            it->second.doid = doid;
        }
        if (m_IsRowMajor != it->second.isRowMajor)
        {
            metaj["M"] = m_IsRowMajor;
            it->second.isRowMajor = m_IsRowMajor;
        }
        if (m_IsLittleEndian != it->second.isLittleEndian)
        {
            metaj["E"] = m_IsLittleEndian;
            it->second.isLittleEndian = m_IsLittleEndian;
        }
        if (GetType<T>() != it->second.type)
        {
            metaj["Y"] = GetType<T>();
            it->second.type = GetType<T>();
        }
        if (varShape != it->second.shape)
        {
            metaj["S"] = varShape;
            it->second.shape = varShape;
        }
    }
    else
    {
        metaj["D"] = doid;
        metaj["M"] = m_IsRowMajor;
        metaj["E"] = m_IsLittleEndian;
        metaj["Y"] = GetType<T>();
        metaj["S"] = varShape;
        m_VarDefaultsMap[varName].doid = doid;
        m_VarDefaultsMap[varName].isRowMajor = m_IsRowMajor;
        m_VarDefaultsMap[varName].isLittleEndian = m_IsLittleEndian;
        m_VarDefaultsMap[varName].type = GetType<T>();
        m_VarDefaultsMap[varName].shape = varShape;
    }

    size_t datasize;

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
                    Zfp<T>(metaj, datasize, inputData, varCount, params);
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
                    Sz<T>(metaj, datasize, inputData, varCount, params);
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
                    BZip2<T>(metaj, datasize, inputData, varCount, params);
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

    std::vector<std::uint8_t> metacbor = nlohmann::json::to_msgpack(metaj);

    uint32_t metasize = metacbor.size();
    size_t totalsize = sizeof(metasize) + metasize + datasize;
    if (m_Buffer->capacity() < m_Position + totalsize)
    {
        m_Buffer->reserve(m_Buffer->capacity() * 2);
    }

    m_Buffer->resize(m_Position + totalsize);

    std::memcpy(m_Buffer->data() + m_Position, &metasize, sizeof(metasize));
    m_Position += sizeof(metasize);

    std::memcpy(m_Buffer->data() + m_Position, metacbor.data(), metasize);
    m_Position += metasize;
    m_TotalMetadataSize += metasize;

    if (compressed)
    {
        std::memcpy(m_Buffer->data() + m_Position, m_CompressBuffer.data(),
                    datasize);
    }
    else
    {
        std::memcpy(m_Buffer->data() + m_Position, inputData, datasize);
    }
    m_Position += datasize;
    m_TotalDataSize += datasize;
}

template <class T>
bool DataManSerializer::Zfp(nlohmann::json &metaj, size_t &datasize,
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
bool DataManSerializer::Sz(nlohmann::json &metaj, size_t &datasize,
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
bool DataManSerializer::BZip2(nlohmann::json &metaj, size_t &datasize,
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

} // namespace format
} // namespace adios2

#endif
