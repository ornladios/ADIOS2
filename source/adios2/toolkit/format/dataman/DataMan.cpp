/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp  classes for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#include "DataMan.tcc"
#include <iostream>

namespace adios2
{
namespace format
{

DataManSerializer::DataManSerializer(size_t size) : m_Position(0)
{
    m_Buffer = std::make_shared<std::vector<char>>();
    m_Buffer->reserve(size);
}

size_t DataManSerializer::GetBufferSize() { return m_Position; }

const std::shared_ptr<std::vector<char>> DataManSerializer::GetBuffer()
{
    return m_Buffer;
}

void DataManDeserializer::Put(std::shared_ptr<std::vector<char>> data)
{
    m_Buffer.push_back(data);
    size_t position = 0;
    m_MaxStep = std::numeric_limits<size_t>::min();
    m_MinStep = std::numeric_limits<size_t>::max();
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
            var.name = metaj["N"].get<std::string>();
            var.type = metaj["Y"].get<std::string>();
            var.shape = metaj["S"].get<Dims>();
            var.count = metaj["C"].get<Dims>();
            var.start = metaj["O"].get<Dims>();
            var.step = metaj["T"].get<size_t>();
            var.size = metaj["I"].get<size_t>();
            var.rank = metaj["R"].get<int>();
            var.position = position;
            var.index = m_Buffer.size() - 1;
            if (position + var.size < data->capacity())
            {
                break;
            }
            m_MetaDataMap[var.step].push_back(std::move(var));
            position += var.size;
        }
        catch (std::exception &e)
        {
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

void DataManDeserializer::Erase(size_t step) { m_MetaDataMap.erase(step); }

size_t DataManDeserializer::MaxStep() { return m_MaxStep; }

size_t DataManDeserializer::MinStep() { return m_MinStep; }

bool DataManDeserializer::Check(size_t step, std::string variable)
{
    const auto &i = m_MetaDataMap.find(step);
    if (i != m_MetaDataMap.end())
    {
        for (const auto &j : i->second)
        {
            if (j.name == variable)
            {
                return true;
            }
        }
    }
    return false;
}

bool DataManDeserializer::Check(size_t step)
{
    const auto &i = m_MetaDataMap.find(step);
    if (i != m_MetaDataMap.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

const std::vector<DataManDeserializer::DataManVar> &
DataManDeserializer::GetMetaData(size_t step)
{
    if (Check(step))
    {
        return m_MetaDataMap[step];
    }
    else
    {
        return m_EmptyVector;
    }
}
}
}
