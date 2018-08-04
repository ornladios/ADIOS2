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
        catch (std::exception &e)
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
    size_t position = 0;
    while (position < data->capacity())
    {
        uint32_t metasize;
        std::memcpy(&metasize, data->data() + position, sizeof(metasize));
        position += sizeof(metasize);
        if (position + metasize > data->size())
        {
            break;
        }
        DataManVar var;
        try
        {
            nlohmann::json metaj =
                nlohmann::json::parse(data->data() + position);
            position += metasize;
            var.isRowMajor = metaj["M"].get<bool>();
            var.isLittleEndian = metaj["E"].get<bool>();
            var.name = metaj["N"].get<std::string>();
            var.type = metaj["Y"].get<std::string>();
            var.shape = metaj["S"].get<Dims>();
            var.count = metaj["C"].get<Dims>();
            var.start = metaj["O"].get<Dims>();
            var.step = metaj["T"].get<size_t>();
            var.size = metaj["I"].get<size_t>();
            var.rank = metaj["R"].get<int>();
            var.doid = metaj["D"].get<std::string>();
            var.position = position;
            var.index = key;

            auto it = metaj.find("Z");
            if (it != metaj.end())
            {
                var.compression = it->get<std::string>();
            }

            for (auto i = metaj.begin(); i != metaj.end(); ++i)
            {
                auto pos = i.key().find(":");
                if (pos != std::string::npos)
                {
                    var.params[i.key().substr(pos + 1)] = i.value();
                }
            }

            if (position + var.size > data->capacity())
            {
                break;
            }
            if (m_MetaDataMap[var.step] == nullptr)
            {
                m_MetaDataMap[var.step] =
                    std::make_shared<std::vector<DataManVar>>();
            }
            m_MetaDataMap[var.step]->push_back(std::move(var));
            position += var.size;
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

} // namespace format
} // namespace adios2
