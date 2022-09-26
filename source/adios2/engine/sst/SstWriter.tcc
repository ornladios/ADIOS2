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

#include "adios2/helper/adiosFunctions.h" //GetDataType<T>
#include <adios2-perfstubs-interface.h>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SstWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    variable.SetData(values);

    if (m_BetweenStepPairs == false)
    {
        helper::Throw<std::logic_error>("Engine", "SstWriter", "PutSyncCommon",
                                        "When using the SST engine in ADIOS2, "
                                        "Put() calls must appear between "
                                        "BeginStep/EndStep pairs");
    }

    if ((Params.MarshalMethod == SstMarshalFFS) ||
        (Params.MarshalMethod == SstMarshalBP5))
    {
        size_t *Shape = NULL;
        size_t *Start = NULL;
        size_t *Count = NULL;
        size_t DimCount = 0;

        if (variable.m_ShapeID == ShapeID::GlobalArray)
        {
            DimCount = variable.m_Shape.size();
            Shape = variable.m_Shape.data();
            Start = variable.m_Start.data();
            Count = variable.m_Count.data();
        }
        else if (variable.m_ShapeID == ShapeID::LocalArray)
        {
            DimCount = variable.m_Count.size();
            Count = variable.m_Count.data();
        }
        if (Params.MarshalMethod == SstMarshalFFS)
        {
            SstFFSMarshal(m_Output, (void *)&variable, variable.m_Name.c_str(),
                          (int)variable.m_Type, variable.m_ElementSize,
                          DimCount, Shape, Count, Start, values);
        }
        else
        {
            if (variable.m_Type == DataType::String)
            {
                std::string &source = *(std::string *)values;
                void *p = &(source[0]);
                m_BP5Serializer->Marshal(
                    (void *)&variable, variable.m_Name.c_str(), variable.m_Type,
                    variable.m_ElementSize, DimCount, Shape, Count, Start, &p,
                    true, nullptr);
            }
            else
            {
                m_BP5Serializer->Marshal(
                    (void *)&variable, variable.m_Name.c_str(), variable.m_Type,
                    variable.m_ElementSize, DimCount, Shape, Count, Start,
                    values, true, nullptr);
            }
        }
    }
    else if (Params.MarshalMethod == SstMarshalBP)
    {
        auto &blockInfo = variable.SetBlockInfo(
            values, m_BP3Serializer->m_MetadataSet.CurrentStep);

        if (!m_BP3Serializer->m_MetadataSet.DataPGIsOpen)
        {
            m_BP3Serializer->PutProcessGroupIndex(
                m_IO.m_Name,
                (m_IO.m_ArrayOrder == ArrayOrdering::RowMajor) ? "C++"
                                                               : "Fortran",
                {"SST"});
        }
        const size_t dataSize =
            helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
            m_BP3Serializer->GetBPIndexSizeInData(variable.m_Name,
                                                  blockInfo.Count);
        format::BP3Base::ResizeResult resizeResult =
            m_BP3Serializer->ResizeBuffer(
                dataSize, "in call to variable " + variable.m_Name +
                              " Put adios2::Mode::Sync");
        if (resizeResult == format::BP3Base::ResizeResult::Failure)
        {
            helper::Throw<std::runtime_error>(
                "Engine", "SstWriter", "PutSyncCommon",
                "Failed to resize BP3 serializer buffer");
        }

        const bool sourceRowMajor =
            (m_IO.m_ArrayOrder == ArrayOrdering::RowMajor);
        m_BP3Serializer->PutVariableMetadata(variable, blockInfo,
                                             sourceRowMajor);
        m_BP3Serializer->PutVariablePayload(variable, blockInfo,
                                            sourceRowMajor);
        variable.m_BlocksInfo.clear();
    }
    else
    {
        helper::Throw<std::invalid_argument>("Engine", "SstWriter",
                                             "PutSyncCommon",
                                             "unknown marshaling method");
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_WRITER_TCC_ */
