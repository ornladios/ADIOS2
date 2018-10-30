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
    m_BP3Serializer.PutVariableMetadata(variable, blockInfo);
    m_BP3Serializer.PutVariablePayload(variable, blockInfo,
                                       helper::IsRowMajor(m_IO.m_HostLanguage));
}

template <class T>
void BP3Writer::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    const typename Variable<T>::Info blockInfo =
        variable.SetBlockInfo(data, CurrentStep());
    m_BP3Serializer.m_DeferredVariables.insert(variable.m_Name);
    m_BP3Serializer.m_DeferredVariablesDataSize += static_cast<size_t>(
        1.05 * helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
        4 *
            m_BP3Serializer.GetBPIndexSizeInData(variable.m_Name,
                                                 blockInfo.Count));
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP3_BP3WRITER_TCC_ */
