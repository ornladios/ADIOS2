/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManDeserializer.cpp Deserializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#include "DataManDeserializer.tcc"

#include <cstring>
#include <iostream>

namespace adios2
{
namespace format
{

DataManDeserializer::DataManDeserializer(const bool isRowMajor,
                                         const bool isLittleEndian)
{
    m_IsRowMajor = isRowMajor;
    m_IsLittleEndian = isLittleEndian;
}

int DataManDeserializer::Put(
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

void DataManDeserializer::Erase(size_t step)
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

size_t DataManDeserializer::MaxStep()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    return m_MaxStep;
}

size_t DataManDeserializer::MinStep()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    return m_MinStep;
}

const std::unordered_map<
    size_t, std::shared_ptr<std::vector<DataManDeserializer::DataManVar>>>
DataManDeserializer::GetMetaData()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    // This meta data map is supposed to be very light weight to return because
    // 1) it only holds shared pointers, and 2) the old steps are removed
    // regularly by the engine.
    return m_MetaDataMap;
}

std::shared_ptr<const std::vector<DataManDeserializer::DataManVar>>
DataManDeserializer::GetMetaData(const size_t step)
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

bool DataManDeserializer::HasOverlap(Dims in_start, Dims in_count,
                                     Dims out_start, Dims out_count) const
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

void DataManDeserializer::GetAttributes(core::IO &io)
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
