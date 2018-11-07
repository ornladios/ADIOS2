/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManSerializer.cpp Serializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#include "DataManSerializer.tcc"

#include "adios2/helper/adiosMPIFunctions.h"

#include <cstring>
#include <iostream>

namespace adios2
{
namespace format
{

DataManSerializer::DataManSerializer(bool isRowMajor,
                                     const bool contiguousMajor,
                                     bool isLittleEndian)
{
    m_IsRowMajor = isRowMajor;
    m_IsLittleEndian = isLittleEndian;
    m_ContiguousMajor = contiguousMajor;
    New(1024);
}

void DataManSerializer::New(size_t size)
{
    // make a new shared object each time because the old shared object could
    // still be alive and needed somewhere in the workflow, for example the
    // queue in transport manager. It will be automatically released when the
    // entire workflow finishes using it.
    m_Metadata = nullptr;
    m_Buffer = std::make_shared<std::vector<char>>();
    m_Buffer->reserve(size);
    m_Position = sizeof(uint64_t) * 2;
}

const std::shared_ptr<std::vector<char>> DataManSerializer::Get()
{
    std::vector<char> metacbor;
    nlohmann::json::to_msgpack(m_Metadata, metacbor);
    size_t metasize = metacbor.size();
    m_Buffer->resize(m_Position + metasize);
    (reinterpret_cast<uint64_t *>(m_Buffer->data()))[0] = m_Position;
    (reinterpret_cast<uint64_t *>(m_Buffer->data()))[1] = metasize;
    std::memcpy(m_Buffer->data() + m_Position, metacbor.data(), metasize);
    return m_Buffer;
}

std::shared_ptr<std::vector<char>>
DataManSerializer::GetAggregatedMetadata(const MPI_Comm mpiComm)
{

    std::vector<char> localJsonCbor;
    nlohmann::json::to_msgpack(m_Metadata, localJsonCbor);
    unsigned int size = localJsonCbor.size();
    unsigned int maxSize;
    MPI_Allreduce(&size, &maxSize, 1, MPI_UNSIGNED, MPI_MAX, mpiComm);
    localJsonCbor.resize(maxSize, '\0');

    int mpiSize;
    int mpiRank;
    MPI_Comm_size(mpiComm, &mpiSize);
    MPI_Comm_rank(mpiComm, &mpiRank);

    std::vector<char> globalJsonStr(mpiSize * maxSize);
    helper::GatherArrays(localJsonCbor.data(), maxSize, globalJsonStr.data(),
                         mpiComm);

    nlohmann::json globalMetadata;
    std::shared_ptr<std::vector<char>> globalJsonCbor = nullptr;
    if (mpiRank == 0)
    {
        for (int i = 0; i < mpiSize; ++i)
        {
            nlohmann::json metaj = nlohmann::json::from_msgpack(
                globalJsonStr.data() + i * maxSize, maxSize);
            for (auto stepMapIt = metaj.begin(); stepMapIt != metaj.end();
                 ++stepMapIt)
            {
                for (auto rankMapIt = stepMapIt.value().begin();
                     rankMapIt != stepMapIt.value().end(); ++rankMapIt)
                {
                    globalMetadata[stepMapIt.key()][rankMapIt.key()] =
                        rankMapIt.value();
                }
            }
        }
        globalJsonCbor = std::make_shared<std::vector<char>>();
        nlohmann::json::to_msgpack(globalMetadata, *globalJsonCbor);
    }

    return globalJsonCbor;
}

float DataManSerializer::GetMetaRatio() { return 0; }

std::shared_ptr<std::vector<char>> DataManSerializer::EndSignal(size_t step)
{
    nlohmann::json j;
    j["FinalStep"] = step;
    std::string s = j.dump() + '\0';
    std::shared_ptr<std::vector<char>> c =
        std::make_shared<std::vector<char>>(s.size());
    std::memcpy(c->data(), s.c_str(), s.size());
    return c;
}

bool DataManSerializer::IsCompressionAvailable(const std::string &method,
                                               const std::string &type,
                                               const Dims &count)
{
    if (method == "zfp")
    {
        if (type == "int" || type == "long" || type == "int32_t" ||
            type == "int64_t" || type == "float" || type == "double")
        {
            if (count.size() <= 3)
            {
                return true;
            }
        }
    }
    else if (method == "sz")
    {
        if (type == "float" || type == "double")
        {
            if (count.size() <= 5)
            {
                return true;
            }
        }
    }
    else if (method == "bzip2")
    {
        if (type == "int" || type == "long" || type == "int32_t" ||
            type == "int64_t" || type == "float" || type == "double")
        {
            return true;
        }
    }
    return false;
}

void DataManSerializer::PutAttributes(core::IO &io, const int rank)
{
    const auto attributesDataMap = io.GetAttributesDataMap();
    for (const auto &attributePair : attributesDataMap)
    {
        const std::string name(attributePair.first);
        const std::string type(attributePair.second.first);
        if (type == "unknown")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Attribute<T> &attribute = *io.InquireAttribute<T>(name);         \
        PutAttribute(attribute, rank);                                         \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    }
}

} // namespace format
} // namespace adios2
