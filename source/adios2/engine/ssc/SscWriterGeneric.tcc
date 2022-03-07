/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriterGeneric.tcc
 *
 *  Created on: Mar 3, 2022
 *      Author: Jason Wang
 */

#include "SscWriterGeneric.h"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

template <>
void SscWriterGeneric::PutDeferredCommon(Variable<std::string> &variable,
                                         const std::string *data)
{
    variable.SetData(data);

    bool found = false;
    for (const auto &b : m_GlobalWritePattern[m_StreamRank])
    {
        if (b.name == variable.m_Name)
        {
            if (b.bufferCount < data->size())
            {
                helper::Throw<std::invalid_argument>(
                    "Engine", "SSCWriter", "PutDeferredCommon",
                    "SSC only accepts fixed length string variables");
            }
            std::memcpy(m_Buffer.data() + b.bufferStart, data->data(),
                        data->size());
            found = true;
        }
    }

    if (!found)
    {
        if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
            m_ReaderSelectionsLocked == false)
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
            helper::Throw<std::invalid_argument>(
                "Engine", "SSCWriter", "PutDeferredCommon",
                "IO pattern changed after locking");
        }
    }
}

template <class T>
void SscWriterGeneric::PutDeferredCommon(Variable<T> &variable, const T *data)
{

    if ((variable.m_ShapeID == ShapeID::GlobalValue ||
         variable.m_ShapeID == ShapeID::LocalValue ||
         variable.m_Type == DataType::String) &&
        m_WriterRank != 0)
    {
        return;
    }

    variable.SetData(data);

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (m_IO.m_ArrayOrder != ArrayOrdering::RowMajor)
    {
        std::reverse(vStart.begin(), vStart.end());
        std::reverse(vCount.begin(), vCount.end());
        std::reverse(vShape.begin(), vShape.end());
    }

    bool found = false;
    for (const auto &b : m_GlobalWritePattern[m_StreamRank])
    {
        if (b.name == variable.m_Name && ssc::AreSameDims(vStart, b.start) &&
            ssc::AreSameDims(vCount, b.count) &&
            ssc::AreSameDims(vShape, b.shape))
        {
            std::memcpy(m_Buffer.data() + b.bufferStart, data, b.bufferCount);
            found = true;
        }
    }

    if (!found)
    {
        if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
            m_ReaderSelectionsLocked == false)
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
            helper::Throw<std::invalid_argument>(
                "Engine", "SSCWriter", "PutDeferredCommon",
                "IO pattern changed after locking");
        }
    }
}
}
}
}
}
