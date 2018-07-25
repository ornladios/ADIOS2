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

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace format
{

template <class T>
void DataManSerializer::Put(const core::Variable<T> &variable,
                            const std::string &doid, const size_t step,
                            const int rank, const Params &params)
{
    Put(variable.GetData(), variable.m_Name, variable.m_Shape, variable.m_Start,
        variable.m_Count, doid, step, rank, params);
}

template <class T>
void DataManSerializer::Put(const T *inputData, const std::string &varName,
                            const Dims &varShape, const Dims &varStart,
                            const Dims &varCount, const std::string &doid,
                            const size_t step, const int rank,
                            const Params &params)
{

    bool compressed = false;

    nlohmann::json metaj;

    metaj["M"] = m_IsRowMajor;
    metaj["E"] = m_IsLittleEndian;
    metaj["N"] = varName;
    metaj["Y"] = GetType<T>();
    metaj["S"] = varShape;
    metaj["C"] = varCount;
    metaj["O"] = varStart;
    metaj["T"] = step;
    metaj["R"] = rank;
    metaj["D"] = doid;

    size_t datasize;

    const auto i = params.find("CompressionMethod");
    if (i != params.end())
    {
        metaj["Z"] = i->second;
        if (i->second == "zfp")
        {
            compressed = Zfp<T>(metaj, datasize, inputData, varCount, params);
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
}

template <class T>
bool DataManSerializer::Zfp(nlohmann::json &metaj, size_t &datasize,
                            const T *inputData, const Dims &varCount,
                            const Params &params)
{
#ifdef ADIOS2_HAVE_ZFP
    float rate = 2.0;
    const auto j = params.find("CompressionRate");
    if (j != params.end())
    {
        rate = stof(j->second);
    }
    metaj["ZR"] = rate;
    Params p = {{"Rate", std::to_string(rate)}};
    core::compress::CompressZfp zfp(p, true);
    m_CompressBuffer.reserve(std::accumulate(varCount.begin(), varCount.end(),
                                             sizeof(T),
                                             std::multiplies<size_t>()));
    try
    {
        datasize = zfp.Compress(inputData, varCount, 4, GetType<T>(),
                                m_CompressBuffer.data(), p);
        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "Got exception " << e.what()
                  << " from ZFP. Turned off compression." << std::endl;
        metaj.erase(metaj.find("Z"));
        metaj.erase(metaj.find("ZR"));
    }
#else
    throw(std::invalid_argument(
        "ZFP compression used but ZFP library is not linked to ADIOS2"));
#endif
    return false;
}
} // namespace format
} // namespace adios2

#endif
