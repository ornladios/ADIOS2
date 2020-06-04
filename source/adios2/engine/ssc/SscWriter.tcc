/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCWRITER_TCC_
#define ADIOS2_ENGINE_SSCWRITER_TCC_

#include "SscWriter.h"
#include "adios2/helper/adiosSystem.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <>
void SscWriter::PutDeferredCommon(Variable<std::string> &variable,
                                  const std::string *data)
{
    TAU_SCOPED_TIMER_FUNC();
    variable.SetData(data);

    bool found = false;
    for (const auto &b : m_GlobalWritePattern[m_StreamRank])
    {
        if (b.name == variable.m_Name)
        {
            if (b.bufferCount < data->size())
            {
                throw(std::runtime_error(
                    "SSC only accepts fixed length string variables"));
            }
            std::memcpy(m_Buffer.data() + b.bufferStart, data->data(),
                        data->size());
            found = true;
        }
    }

    if (!found)
    {
        if (m_CurrentStep == 0)
        {
            m_GlobalWritePattern[m_StreamRank].emplace_back();
            auto &b = m_GlobalWritePattern[m_StreamRank].back();
            b.name = variable.m_Name;
            b.type = DataType::String;
            b.shapeId = variable.m_ShapeID;
            b.shape = variable.m_Shape;
            b.start = variable.m_Start;
            b.count = variable.m_Count;
            b.bufferStart = m_Buffer.size();
            b.bufferCount = data->size();
            m_Buffer.resize(b.bufferStart + b.bufferCount);
            std::memcpy(m_Buffer.data() + b.bufferStart, data->data(),
                        data->size());
            b.value.resize(data->size());
            std::memcpy(b.value.data(), data->data(), data->size());
        }
        else
        {
            throw std::runtime_error("ssc only accepts fixed IO pattern");
        }
    }
}

template <class T>
void SscWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();

    variable.SetData(data);

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (!helper::IsRowMajor(m_IO.m_HostLanguage))
    {
        std::reverse(vStart.begin(), vStart.end());
        std::reverse(vCount.begin(), vCount.end());
        std::reverse(vShape.begin(), vShape.end());
    }

    bool found = false;
    for (const auto &b : m_GlobalWritePattern[m_StreamRank])
    {
        if (b.name == variable.m_Name and ssc::AreSameDims(vStart, b.start) and
            ssc::AreSameDims(vCount, b.count) and
            ssc::AreSameDims(vShape, b.shape))
        {
            std::memcpy(m_Buffer.data() + b.bufferStart, data, b.bufferCount);
            found = true;
        }
    }

    if (!found)
    {
        if (m_CurrentStep == 0)
        {
            m_GlobalWritePattern[m_StreamRank].emplace_back();
            auto &b = m_GlobalWritePattern[m_StreamRank].back();
            b.name = variable.m_Name;
            b.type = helper::GetDataType<T>();
            b.shapeId = variable.m_ShapeID;
            b.shape = vShape;
            b.start = vStart;
            b.count = vCount;
            b.bufferStart = m_Buffer.size();
            b.bufferCount = ssc::TotalDataSize(b.count, b.type, b.shapeId);
            m_Buffer.resize(b.bufferStart + b.bufferCount);
            std::memcpy(m_Buffer.data() + b.bufferStart, data, b.bufferCount);
            if (b.shapeId == ShapeID::GlobalValue ||
                b.shapeId == ShapeID::LocalValue)
            {
                b.value.resize(sizeof(T));
                std::memcpy(b.value.data(), data, b.bufferCount);
            }
        }
        else
        {
            throw std::runtime_error("ssc only accepts fixed IO pattern");
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCWRITER_TCC_
