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

const std::shared_ptr<std::vector<char>> DataManSerializer::GetPack()
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

int DataManSerializer::PutPack(
    const std::shared_ptr<const std::vector<char>> data)
{
    // check if is control signal
    if (data->size() < 128)
    {
        try
        {
            nlohmann::json metaj = nlohmann::json::parse(data->data());
            size_t finalStep = metaj["FinalStep"];
            return finalStep;
        }
        catch (std::exception)
        {
        }
    }

    // if not control signal then go through standard deserialization
    int key = rand();
    std::lock_guard<std::mutex> l(m_Mutex);
    while (m_BufferMap.count(key) > 0)
    {
        key = rand();
    }
    m_BufferMap[key] = data;

    uint64_t metaPosition =
        (reinterpret_cast<const uint64_t *>(data->data()))[0];
    uint64_t metaSize = (reinterpret_cast<const uint64_t *>(data->data()))[1];

    nlohmann::json metaJ =
        nlohmann::json::from_msgpack(data->data() + metaPosition, metaSize);

    for (auto stepMapIt = metaJ.begin(); stepMapIt != metaJ.end(); ++stepMapIt)
    {
        if (stepMapIt.key() == "G" || stepMapIt.key() == "A")
        {
            for (const auto &rankVec : stepMapIt.value())
            {
                for (const auto &gVar : rankVec)
                {
                    m_GlobalVars[gVar["N"].get<std::string>()] = gVar;
                }
            }
            continue;
        }

        for (auto rankMapIt = stepMapIt.value().begin();
             rankMapIt != stepMapIt.value().end(); ++rankMapIt)
        {
            for (const auto &varBlock : rankMapIt.value())
            {
                DataManVar var;
                try
                {
                    // compulsory properties
                    var.step = stoull(stepMapIt.key());
                    var.name = varBlock["N"].get<std::string>();
                    var.start = varBlock["O"].get<Dims>();
                    var.count = varBlock["C"].get<Dims>();
                    var.size = varBlock["I"].get<size_t>();

                    // optional properties

                    auto itJson = varBlock.find("D");
                    if (itJson != varBlock.end())
                    {
                        var.doid = itJson->get<std::string>();
                    }

                    itJson = varBlock.find("M");
                    if (itJson != varBlock.end())
                    {
                        var.isRowMajor = itJson->get<bool>();
                    }

                    itJson = varBlock.find("E");
                    if (itJson != varBlock.end())
                    {
                        var.isLittleEndian = itJson->get<bool>();
                    }

                    itJson = varBlock.find("Y");
                    if (itJson != varBlock.end())
                    {
                        var.type = itJson->get<std::string>();
                    }

                    itJson = varBlock.find("S");
                    if (itJson != varBlock.end())
                    {
                        var.shape = itJson->get<Dims>();
                    }

                    var.position = varBlock["P"].get<size_t>();
                    var.index = key;

                    auto it = varBlock.find("Z");
                    if (it != varBlock.end())
                    {
                        var.compression = it->get<std::string>();
                    }

                    for (auto i = varBlock.begin(); i != varBlock.end(); ++i)
                    {
                        auto pos = i.key().find(":");
                        if (pos != std::string::npos)
                        {
                            var.params[i.key().substr(pos + 1)] = i.value();
                        }
                    }

                    if (m_MetaDataMap[var.step] == nullptr)
                    {
                        m_MetaDataMap[var.step] =
                            std::make_shared<std::vector<DataManVar>>();
                    }
                    m_MetaDataMap[var.step]->emplace_back(std::move(var));
                }
                catch (std::exception &e)
                {
                    std::cout << e.what() << std::endl;
                    return -1;
                }
                if (m_MaxStep < var.step)
                {
                    m_MaxStep = var.step;
                }
                if (m_MinStep > var.step)
                {
                    m_MinStep = var.step;
                }
            }
        }
    }
    return 0;
}

void DataManSerializer::Erase(size_t step)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    const auto &varVec = m_MetaDataMap.find(step);
    // if metadata map has this step
    if (varVec != m_MetaDataMap.end())
    {
        // loop for all vars in this step of metadata map
        for (const auto &var : *varVec->second)
        {
            bool toDelete = true;
            // loop for any steps larger than the current step and smaller than
            // the max step
            for (size_t checkingStep = step + 1; checkingStep <= m_MaxStep;
                 ++checkingStep)
            {
                // find this step in metadata map
                const auto &checkingVarVec = m_MetaDataMap.find(checkingStep);
                if (checkingVarVec != m_MetaDataMap.end())
                {
                    // loop for all vars in var vector
                    for (const auto &checkingVar : *checkingVarVec->second)
                    {
                        // if any DataManVar for the current step being deleted
                        // contains the same raw buffer index as any future
                        // steps contain, then don't delete
                        if (checkingVar.index == var.index)
                        {
                            toDelete = false;
                        }
                    }
                }
            }
            if (toDelete)
            {
                m_BufferMap.erase(var.index);
            }
        }
    }
    m_MetaDataMap.erase(step);
    m_MinStep = step + 1;
}

const std::unordered_map<
    size_t, std::shared_ptr<std::vector<DataManSerializer::DataManVar>>>
DataManSerializer::GetMetaData()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    // This meta data map is supposed to be very light weight to return because
    // 1) it only holds shared pointers, and 2) the old steps are removed
    // regularly by the engine.
    return m_MetaDataMap;
}

std::shared_ptr<const std::vector<DataManSerializer::DataManVar>>
DataManSerializer::GetMetaData(const size_t step)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    const auto &i = m_MetaDataMap.find(step);
    if (i != m_MetaDataMap.end())
    {
        return m_MetaDataMap[step];
    }
    else
    {
        return nullptr;
    }
}

bool DataManSerializer::HasOverlap(Dims in_start, Dims in_count, Dims out_start,
                                   Dims out_count) const
{
    for (size_t i = 0; i < in_start.size(); ++i)
    {
        if (in_start[i] > out_start[i] + out_count[i] ||
            out_start[i] > in_start[i] + in_count[i])
        {
            return false;
        }
    }
    return true;
}

void DataManSerializer::GetAttributes(core::IO &io)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    for (const auto &j : m_GlobalVars)
    {
        const std::string type(j["Y"].get<std::string>());
        if (type == "unknown")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        if (j["V"].get<bool>())                                                \
        {                                                                      \
            io.DefineAttribute<T>(j["N"].get<std::string>(), j["G"].get<T>()); \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            io.DefineAttribute<T>(j["N"].get<std::string>(),                   \
                                  j["G"].get<std::vector<T>>().data(),         \
                                  j["G"].get<std::vector<T>>().size());        \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    }
}

} // namespace format
} // namespace adios2
