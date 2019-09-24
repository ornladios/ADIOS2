/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Writer.tcc implementation of template functions with known type
 *
 *  Created on: May 22, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#ifndef ADIOS2_ENGINE_BP3_BP3WRITER_TCC_
#define ADIOS2_ENGINE_BP3_BP3WRITER_TCC_

#include "BP3Writer.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void BP3Writer::PutCommon(Variable<T> &variable,
                          typename Variable<T>::Span &span,
                          const size_t /*bufferID*/, const T &value)
{
    // if first timestep Write create a new pg index
    if (!m_BP3Serializer.m_MetadataSet.DataPGIsOpen)
    {
        m_BP3Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }
    const typename Variable<T>::Info &blockInfo =
        variable.SetBlockInfo(nullptr, CurrentStep());
    m_BP3Serializer.m_DeferredVariables.insert(variable.m_Name);

    const size_t dataSize =
        helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
        m_BP3Serializer.GetBPIndexSizeInData(variable.m_Name, blockInfo.Count);

    const format::BP3Base::ResizeResult resizeResult =
        m_BP3Serializer.ResizeBuffer(dataSize, "in call to variable " +
                                                   variable.m_Name + " Put");

    if (m_DebugMode && resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        throw std::invalid_argument(
            "ERROR: returning a Span can't trigger "
            "buffer reallocation in BP3 engine, remove "
            "MaxBufferSize parameter, in call to Put\n");
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);
    m_BP3Serializer.PutVariableMetadata(variable, blockInfo, sourceRowMajor,
                                        &span);
    span.m_Value = value;
    m_BP3Serializer.PutVariablePayload(variable, blockInfo, sourceRowMajor,
                                       &span);
}

template <class T>
void BP3Writer::PutSyncCommon(Variable<T> &variable,
                              const typename Variable<T>::Info &blockInfo)
{
    // if first timestep Write create a new pg index
    if (!m_BP3Serializer.m_MetadataSet.DataPGIsOpen)
    {
        m_BP3Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }

    const size_t dataSize =
        helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
        m_BP3Serializer.GetBPIndexSizeInData(variable.m_Name, blockInfo.Count);

    const format::BP3Base::ResizeResult resizeResult =
        m_BP3Serializer.ResizeBuffer(dataSize, "in call to variable " +
                                                   variable.m_Name + " Put");

    if (resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        DoFlush(false);
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);

        // new group index for incoming variable
        m_BP3Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);
    m_BP3Serializer.PutVariableMetadata(variable, blockInfo, sourceRowMajor);
    m_BP3Serializer.PutVariablePayload(variable, blockInfo, sourceRowMajor);
}

template <class T>
void BP3Writer::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    if (variable.m_SingleValue)
    {
        DoPutSync(variable, data);
        return;
    }

    const typename Variable<T>::Info blockInfo =
        variable.SetBlockInfo(data, CurrentStep());
    m_BP3Serializer.m_DeferredVariables.insert(variable.m_Name);
    m_BP3Serializer.m_DeferredVariablesDataSize += static_cast<size_t>(
        1.05 * helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
        4 * m_BP3Serializer.GetBPIndexSizeInData(variable.m_Name,
                                                 blockInfo.Count));
}

template <class T>
T *BP3Writer::BufferDataCommon(const size_t payloadPosition,
                               const size_t /*bufferID*/) noexcept
{
    T *data = reinterpret_cast<T *>(m_BP3Serializer.m_Data.m_Buffer.data() +
                                    payloadPosition);
    return data;
}

template <class T>
void BP3Writer::PerformPutCommon(Variable<T> &variable)
{
    for (size_t b = 0; b < variable.m_BlocksInfo.size(); ++b)
    {
        auto itSpanBlock = variable.m_BlocksSpan.find(b);
        if (itSpanBlock == variable.m_BlocksSpan.end())
        {
            PutSyncCommon(variable, variable.m_BlocksInfo[b]);
        }
        else
        {
            m_BP3Serializer.PutSpanMetadata(variable, itSpanBlock->second);
        }
    }

    variable.m_BlocksInfo.clear();
    variable.m_BlocksSpan.clear();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP3_BP3WRITER_TCC_ */
