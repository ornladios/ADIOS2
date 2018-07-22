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
bool DataManSerializer::Put(const core::Variable<T> &variable,
                            const std::string doid, const size_t step,
                            const int rank, const Params &params)
{

    bool compressed = false;

    nlohmann::json metaj;

    metaj["M"] = m_IsRowMajor;
    metaj["E"] = m_IsLittleEndian;
    metaj["N"] = variable.m_Name;
    metaj["Y"] = variable.m_Type;
    metaj["S"] = variable.m_Shape;
    metaj["C"] = variable.m_Count;
    metaj["O"] = variable.m_Start;
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
            compressed = Zfp(metaj, datasize, variable, params);
        }
        else
        {
            throw(std::invalid_argument("Compression method " + i->second +
                                        " not supported."));
        }
    }

    if (compressed == false)
    {
        datasize = variable.PayloadSize();
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
        std::memcpy(m_Buffer->data() + m_Position, variable.GetData(),
                    datasize);
    }
    m_Position += datasize;

    return true;
}

template <class T>
bool DataManSerializer::Zfp(nlohmann::json &metaj, size_t &datasize,
                            const core::Variable<T> &variable,
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
    m_CompressBuffer.reserve(variable.PayloadSize());
    try
    {
        datasize = zfp.Compress(variable.GetData(), variable.m_Count, 4,
                                variable.m_Type, m_CompressBuffer.data(), p);
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
