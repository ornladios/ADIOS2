/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.tcc  classes for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMAN_TCC_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMAN_TCC_

#include "DataMan.h"

#include <iostream>

namespace adios2
{
namespace format
{

template <class T>
bool DataManSerializer::Put(Variable<T> &variable, size_t step, int rank)
{
    nlohmann::json metaj;
    metaj["N"] = variable.m_Name;
    metaj["Y"] = variable.m_Type;
    metaj["S"] = variable.m_Shape;
    metaj["C"] = variable.m_Count;
    metaj["O"] = variable.m_Start;
    metaj["T"] = step;
    metaj["I"] = variable.PayloadSize();
    metaj["R"] = rank;
    std::string metastr = metaj.dump() + '\0';
    uint32_t metasize = metastr.size();
    size_t datasize = variable.PayloadSize();
    size_t totalsize = sizeof(metasize) + metasize + datasize;
    if (m_Buffer->capacity() < m_Position + totalsize)
    {
        return true;
    }

    m_Buffer->resize(m_Position + totalsize);

    std::memcpy(m_Buffer->data() + m_Position, &metasize, sizeof(metasize));
    m_Position += sizeof(metasize);

    std::memcpy(m_Buffer->data() + m_Position, metastr.c_str(), metasize);
    m_Position += metasize;

    std::memcpy(m_Buffer->data() + m_Position, variable.GetData(), datasize);
    m_Position += datasize;
    return false;
}

template <class T>
int DataManDeserializer::Get(Variable<T> &variable, size_t step)
{

    int ret;

    std::shared_ptr<std::vector<DataManVar>> vec = nullptr;

    m_MutexMetaData.lock();
    const auto &i = m_MetaDataMap.find(step);
    if (i == m_MetaDataMap.end())
    {
        ret = -1; // step not found
    }
    else
    {
        vec = i->second;
        ret = -2; // step found but variable not found
    }
    m_MutexMetaData.unlock();

    if (vec != nullptr)
    {
        for (const auto &j : *vec)
        {
            if (j.name == variable.m_Name)
            {
                // Get the shared pointer first and then copy memory. This is
                // done in order to avoid expensive memory copy operations
                // happening inside the lock. Once the shared pointer is
                // assigned to k, its life cycle in m_BufferMap does not matter
                // any more. So even if it is released somewhere else the memory
                // is still valid until k dies.
                m_MutexBuffer.lock();
                std::shared_ptr<std::vector<char>> k = m_BufferMap[j.index];
                m_MutexBuffer.unlock();
                std::memcpy(variable.GetData(), k->data() + j.position, j.size);
                ret = 0; // data obtained
            }
        }
    }

    return ret;
}
}
}

#endif
