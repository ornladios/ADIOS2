/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Writer.tcc implementation of template functions with known type
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */
#ifndef ADIOS2_ENGINE_BP4_BP4WRITER_TCC_
#define ADIOS2_ENGINE_BP4_BP4WRITER_TCC_

#include "BP4Writer.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void BP4Writer::PutSyncCommon(Variable<T> &variable,
                              const typename Variable<T>::Info &blockInfo)
{
    // if first timestep Write create a new pg index
    if (!m_BP4Serializer.m_MetadataSet.DataPGIsOpen)
    {
        m_BP4Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }

    const size_t dataSize =
        helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
        m_BP4Serializer.GetBPIndexSizeInData(variable.m_Name, blockInfo.Count);

    const format::BP4Base::ResizeResult resizeResult =
        m_BP4Serializer.ResizeBuffer(dataSize, "in call to variable " +
                                                   variable.m_Name + " Put");

    if (resizeResult == format::BP4Base::ResizeResult::Flush)
    {
        DoFlush(false);
        m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Data);

        // new group index for incoming variable
        m_BP4Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);
    m_BP4Serializer.PutVariableMetadata(variable, blockInfo, sourceRowMajor);
    m_BP4Serializer.PutVariablePayload(variable, blockInfo, sourceRowMajor);
}

template <class T>
void BP4Writer::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    if (variable.m_SingleValue)
    {
        DoPutSync(variable, data);
        return;
    }

    const typename Variable<T>::Info blockInfo =
        variable.SetBlockInfo(data, CurrentStep());
    m_BP4Serializer.m_DeferredVariables.insert(variable.m_Name);
    m_BP4Serializer.m_DeferredVariablesDataSize += static_cast<size_t>(
        1.05 * helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
        4 * m_BP4Serializer.GetBPIndexSizeInData(variable.m_Name,
                                                 blockInfo.Count));
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4WRITER_TCC_ */
