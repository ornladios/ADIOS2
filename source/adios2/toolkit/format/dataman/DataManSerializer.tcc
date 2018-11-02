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
                            const int rank, const std::string &address,
                            const Params &params)
{
    Put(variable.GetData(), variable.m_Name, variable.m_Shape, variable.m_Start,
        variable.m_Count, doid, step, rank, address, params);
}

template <class T>
void DataManSerializer::Put(const T *inputData, const std::string &varName,
                            const Dims &varShape, const Dims &varStart,
                            const Dims &varCount, const std::string &doid,
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
    metaj["P"] = m_Position;

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

    if (m_Buffer->capacity() < m_Position + datasize)
    {
        m_Buffer->reserve((m_Position + datasize) * 2);
    }

    m_Buffer->resize(m_Position + datasize);

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

    m_Metadata[std::to_string(step)][std::to_string(rank)].emplace_back(metaj);
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

template <class T>
void DataManSerializer::PutAttribute(const core::Attribute<T> &attribute,
                                     const int rank)
{
    m_Metadata["A"][std::to_string(rank)].emplace_back();
    auto &j = m_Metadata["A"][std::to_string(rank)].back();
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

} // namespace format
} // namespace adios2

#endif
