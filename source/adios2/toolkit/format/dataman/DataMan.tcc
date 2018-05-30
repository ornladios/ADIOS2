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
    const auto &i = m_MetaDataMap.find(step);
    if (i == m_MetaDataMap.end())
    {
        ret = -1;
    }
    else
    {
        for (const auto &j : i->second)
        {
            if (j.name == variable.m_Name && j.step == step)
            {
                std::memcpy(variable.GetData(),
                            m_Buffer[j.index]->data() + j.position, j.size);
                ret = 1;
            }
        }
    }

    return ret;
}
}
}

#endif
