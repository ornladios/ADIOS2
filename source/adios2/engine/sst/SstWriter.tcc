/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstWriter.tcc
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#ifndef ADIOS2_ENGINE_SST_SST_WRITER_TCC_
#define ADIOS2_ENGINE_SST_SST_WRITER_TCC_

#include "SstWriter.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SstWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);

    if (m_BetweenStepPairs == false)
    {
        throw std::logic_error("ERROR: When using the SST engine in ADIOS2, "
                               "Put() calls must appear between "
                               "BeginStep/EndStep pairs");
    }
    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    if (m_MarshalMethod == SstMarshalFFS)
    {
        SstFFSMarshal(m_Output, (void *)&variable, variable.m_Name.c_str(),
                      variable.m_Type.c_str(), variable.m_ElementSize,
                      variable.m_Shape.size(), variable.m_Shape.data(),
                      variable.m_Count.data(), variable.m_Start.data(), values);
    }
    else if (m_MarshalMethod == SstMarshalBP)
    {
        auto &blockInfo = variable.SetBlockInfo(
            values, m_BP3Serializer->m_MetadataSet.CurrentStep);

        if (!m_BP3Serializer->m_MetadataSet.DataPGIsOpen)
        {
            m_BP3Serializer->PutProcessGroupIndex(m_IO.m_Name,
                                                  m_IO.m_HostLanguage, {"SST"});
        }
        const size_t dataSize =
            helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
            m_BP3Serializer->GetBPIndexSizeInData(variable.m_Name,
                                                  blockInfo.Count);
        format::BP3Base::ResizeResult resizeResult =
            m_BP3Serializer->ResizeBuffer(
                dataSize, "in call to variable " + variable.m_Name +
                              " Put adios2::Mode::Sync");
        const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);
        m_BP3Serializer->PutVariableMetadata(variable, blockInfo,
                                             sourceRowMajor);
        m_BP3Serializer->PutVariablePayload(variable, blockInfo,
                                            sourceRowMajor);
        variable.m_BlocksInfo.clear();
    }
    else
    {
        throw std::invalid_argument("ERROR: unknown marshaling method \n");
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_WRITER_TCC_ */
