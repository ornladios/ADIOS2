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

template <>
inline void DataManSerializer::CalculateMinMax<std::complex<float>>(
    const std::complex<float> *data, const Dims &count, nlohmann::json &metaj)
{
}

template <>
inline void DataManSerializer::CalculateMinMax<std::complex<double>>(
    const std::complex<double> *data, const Dims &count, nlohmann::json &metaj)
{
}

template <typename T>
void DataManSerializer::CalculateMinMax(const T *data, const Dims &count,
                                        nlohmann::json &metaj)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    T max = std::numeric_limits<T>::min();
    T min = std::numeric_limits<T>::max();

    for (size_t j = 0; j < size; ++j)
    {
        T value = data[j];
        if (value > max)
        {
            max = value;
        }
        if (value < min)
        {
            min = value;
        }
    }

    std::vector<char> vectorValue(sizeof(T));

    reinterpret_cast<T *>(vectorValue.data())[0] = max;
    metaj["+"] = vectorValue;

    reinterpret_cast<T *>(vectorValue.data())[0] = min;
    metaj["-"] = vectorValue;
}

template <class T>
void DataManSerializer::PutVar(const core::Variable<T> &variable,
                               const std::string &doid, const size_t step,
                               const int rank, const std::string &address,
                               const Params &params, VecPtr localBuffer,
                               JsonPtr metadataJson)
{
    PutVar(variable.GetData(), variable.m_Name, variable.m_Shape,
           variable.m_Start, variable.m_Count, variable.m_MemoryStart,
           variable.m_MemoryCount, doid, step, rank, address, params,
           localBuffer, metadataJson);
}

template <class T>
void DataManSerializer::PutVar(const T *inputData, const std::string &varName,
                               const Dims &varShape, const Dims &varStart,
                               const Dims &varCount, const Dims &varMemStart,
                               const Dims &varMemCount, const std::string &doid,
                               const size_t step, const int rank,
                               const std::string &address, const Params &params,
                               VecPtr localBuffer, JsonPtr metadataJson)
{
    Log(1,
        "DataManSerializer::PutVar begin with Step " + std::to_string(step) +
            " Var " + varName,
        true, true);

    if (localBuffer == nullptr)
    {
        localBuffer = m_LocalBuffer;
    }

    nlohmann::json metaj;

    metaj["A"] = address;
    metaj["N"] = varName;
    metaj["O"] = varStart;
    metaj["C"] = varCount;
    metaj["S"] = varShape;
    metaj["Y"] = helper::GetType<T>();
    metaj["P"] = localBuffer->size();

    if (m_EnableStat)
    {
        CalculateMinMax(inputData, varCount, metaj);
    }

    if (not m_IsRowMajor)
    {
        metaj["M"] = m_IsRowMajor;
    }
    if (not m_IsLittleEndian)
    {
        metaj["E"] = m_IsLittleEndian;
    }

    size_t datasize = 0;
    bool compressed = false;
    if (params.empty() == false)
    {
        const auto i = params.find("CompressionMethod");
        if (i != params.end())
        {
            std::string compressionMethod = i->second;
            std::transform(compressionMethod.begin(), compressionMethod.end(),
                           compressionMethod.begin(), ::tolower);
            if (compressionMethod == "zfp")
            {
                if (IsCompressionAvailable(compressionMethod,
                                           helper::GetType<T>(), varCount))
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
                if (IsCompressionAvailable(compressionMethod,
                                           helper::GetType<T>(), varCount))
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
                if (IsCompressionAvailable(compressionMethod,
                                           helper::GetType<T>(), varCount))
                {
                    compressed = PutBZip2<T>(metaj, datasize, inputData,
                                             varCount, params);
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
    }

    if (compressed == false)
    {
        datasize = std::accumulate(varCount.begin(), varCount.end(), sizeof(T),
                                   std::multiplies<size_t>());
    }
    metaj["I"] = datasize;

    if (localBuffer->capacity() < localBuffer->size() + datasize)
    {
        localBuffer->reserve((localBuffer->size() + datasize) * 2);
    }

    localBuffer->resize(localBuffer->size() + datasize);

    if (compressed)
    {
        std::memcpy(localBuffer->data() + localBuffer->size() - datasize,
                    m_CompressBuffer.data(), datasize);
    }
    else
    {
        std::memcpy(localBuffer->data() + localBuffer->size() - datasize,
                    inputData, datasize);
    }

    if (metadataJson == nullptr)
    {
        m_MetadataJson[std::to_string(step)][std::to_string(rank)].emplace_back(
            std::move(metaj));
    }
    else
    {
        (*metadataJson)[std::to_string(step)][std::to_string(rank)]
            .emplace_back(std::move(metaj));
    }

    if (m_Verbosity >= 100)
    {
        Log(100, "DataManSerializer::PutVar printing data", true, true);
        for (size_t i = 0; i < datasize / sizeof(T); ++i)
        {
            std::cout << inputData[i] << "  ";
        }
        std::cout << std::endl;
    }

    Log(1,
        "DataManSerializer::PutVar end with Step " + std::to_string(step) +
            " Var " + varName,
        true, true);
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
        datasize =
            compressor.Compress(inputData, varCount, 4, helper::GetType<T>(),
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
        datasize =
            compressor.Compress(inputData, varCount, 4, helper::GetType<T>(),
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
        datasize =
            compressor.Compress(inputData, varCount, 4, helper::GetType<T>(),
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
void DataManSerializer::PutAttribute(const core::Attribute<T> &attribute)
{
    nlohmann::json staticVar;
    staticVar["N"] = attribute.m_Name;
    staticVar["Y"] = attribute.m_Type;
    staticVar["V"] = attribute.m_IsSingleValue;
    if (attribute.m_IsSingleValue)
    {
        staticVar["G"] = attribute.m_DataSingleValue;
    }
    else
    {
        staticVar["G"] = attribute.m_DataArray;
    }

    m_StaticDataJsonMutex.lock();
    m_StaticDataJson["S"].emplace_back(std::move(staticVar));
    m_StaticDataJsonMutex.unlock();
}

template <class T>
int DataManSerializer::GetVar(T *outputData, const std::string &varName,
                              const Dims &varStart, const Dims &varCount,
                              const size_t step, const Dims &varMemStart,
                              const Dims &varMemCount)
{

    DmvVecPtr vec = nullptr;

    {
        std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
        const auto &i = m_DataManVarMap.find(step);
        if (i == m_DataManVarMap.end())
        {
            return -1; // step not found
        }
        else
        {
            vec = i->second;
        }
    }

    if (vec == nullptr)
    {
        return -2; // step found but variable not found
    }

    char *input_data = nullptr;

    for (const auto &j : *vec)
    {
        if (j.name == varName)
        {
            if (j.buffer == nullptr)
            {
                continue;
            }
            else
            {
                input_data = reinterpret_cast<char *>(j.buffer->data());
            }
            if (j.compression == "zfp")
            {
#ifdef ADIOS2_HAVE_ZFP
                core::compress::CompressZfp decompressor(j.params, true);
                std::vector<char> decompressBuffer;
                size_t datasize =
                    std::accumulate(j.count.begin(), j.count.end(), sizeof(T),
                                    std::multiplies<size_t>());

                decompressBuffer.reserve(datasize);
                try
                {
                    decompressor.Decompress(j.buffer->data() + j.position,
                                            j.size, decompressBuffer.data(),
                                            j.count, j.type, j.params);
                }
                catch (std::exception &e)
                {
                    std::cout << "[DataManDeserializer::Get] Zfp "
                                 "decompression failed with exception: "
                              << e.what() << std::endl;
                    return -4; // decompression failed
                }

                input_data = decompressBuffer.data();
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
                    std::accumulate(j.count.begin(), j.count.end(), sizeof(T),
                                    std::multiplies<size_t>());

                decompressBuffer.reserve(datasize);
                try
                {
                    decompressor.Decompress(j.buffer->data() + j.position,
                                            j.size, decompressBuffer.data(),
                                            j.count, j.type, j.params);
                }
                catch (std::exception &e)
                {
                    std::cout << "[DataManDeserializer::Get] Zfp "
                                 "decompression failed with exception: "
                              << e.what() << std::endl;
                    return -4; // decompression failed
                }
                input_data = decompressBuffer.data();
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
                    std::accumulate(j.count.begin(), j.count.end(), sizeof(T),
                                    std::multiplies<size_t>());

                decompressBuffer.reserve(datasize);
                try
                {
                    decompressor.Decompress(j.buffer->data() + j.position,
                                            j.size, decompressBuffer.data(),
                                            datasize);
                }
                catch (std::exception &e)
                {
                    std::cout << "[DataManDeserializer::Get] Zfp "
                                 "decompression failed with exception: "
                              << e.what() << std::endl;
                    return -4; // decompression failed
                }
                input_data = decompressBuffer.data();
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
                if (m_Verbosity >= 5)
                {
                    std::cout
                        << "DataManSerializer calling NdCopy with input start ";
                    for (auto s : j.start)
                    {
                        std::cout << s << ", ";
                    }
                    std::cout << "count ";
                    for (auto s : j.count)
                    {
                        std::cout << s << ", ";
                    }
                    std::cout << "output start ";
                    for (auto s : varStart)
                    {
                        std::cout << s << ", ";
                    }
                    std::cout << "count ";
                    for (auto s : varCount)
                    {
                        std::cout << s << ", ";
                    }
                    std::cout << std::endl;
                }
                if (j.start.size() > 0 && j.start.size() == j.count.size() &&
                    j.start.size() == varStart.size() &&
                    j.start.size() == varCount.size())
                {
                    if (m_ContiguousMajor)
                    {
                        helper::NdCopy<T>(input_data + j.position, j.start,
                                          j.count, true, j.isLittleEndian,
                                          reinterpret_cast<char *>(outputData),
                                          varStart, varCount, true,
                                          m_IsLittleEndian, j.start, j.count,
                                          varMemStart, varMemCount);
                    }
                    else
                    {
                        helper::NdCopy<T>(
                            input_data + j.position, j.start, j.count,
                            j.isRowMajor, j.isLittleEndian,
                            reinterpret_cast<char *>(outputData), varStart,
                            varCount, m_IsRowMajor, m_IsLittleEndian, j.start,
                            j.count, varMemStart, varMemCount);
                    }
                    if (m_Verbosity >= 100)
                    {
                        size_t datasize =
                            std::accumulate(j.count.begin(), j.count.end(), 1,
                                            std::multiplies<size_t>());
                        std::cout
                            << "DataManSerializer::GetVar printing input data"
                            << std::endl;
                        for (size_t i = 0; i < datasize; ++i)
                        {
                            std::cout << (reinterpret_cast<T *>(input_data +
                                                                j.position))[i]
                                      << "  ";
                        }
                        std::cout << std::endl;
                    }
                }
            }
        }
    }
    if (m_Verbosity >= 100)
    {
        size_t datasize = std::accumulate(varCount.begin(), varCount.end(), 1,
                                          std::multiplies<size_t>());
        std::cout << "DataManSerializer::GetVar printing output data"
                  << std::endl;
        for (size_t i = 0; i < datasize; ++i)
        {
            std::cout << outputData[i] << "  ";
        }
        std::cout << std::endl;
    }
    return 0;
}

} // namespace format
} // namespace adios2

#endif
