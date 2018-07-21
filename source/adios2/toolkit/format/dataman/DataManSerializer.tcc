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

#ifdef ADIOS2_HAVE_ZFP
template <class T>
bool DataManSerializer::PutZfp(core::Variable<T> &variable, std::string doid,
                               size_t step, int rank, const Params &params)
{
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

} // namespace format
} // namespace adios2

#endif
