/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_TCC_

#include "DataManWriter.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void DataManWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{

    variable.SetData(values);

    if (variable.m_Shape.empty())
    {
        variable.m_Shape = variable.m_Count;
    }
    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    if (m_Format == "bp")
    {
        PutSyncCommonBP(
            variable, variable.SetBlockInfo(
                          values, m_BP3Serializer->m_MetadataSet.CurrentStep));
        variable.m_BlocksInfo.clear();
    }
    else if (m_Format == "dataman")
    {
        PutSyncCommonDataMan(variable, values);
    }
    else if (m_Format == "binary")
    {
    }
    else
    {
        throw(std::invalid_argument(
            "[DataManWriter::PutSyncCommon] invalid format " + m_Format));
    }
}

template <class T>
void DataManWriter::PutSyncCommonDataMan(Variable<T> &variable, const T *values)
{
    for (size_t i = 0; i < m_TransportChannels; ++i)
    {
        m_DataManSerializer[i]->Put(variable, m_Name, CurrentStep(), m_MPIRank,
                                    m_IO.m_TransportsParameters[i]);
    }
}

template <class T>
void DataManWriter::PutSyncCommonBP(Variable<T> &variable,
                                    const typename Variable<T>::Info &blockInfo)
{
    // add bp serialization
    // if first timestep Write create a new pg index
    if (!m_BP3Serializer->m_MetadataSet.DataPGIsOpen)
    {
        m_BP3Serializer->PutProcessGroupIndex(m_IO.m_Name, m_IO.m_HostLanguage,
                                              {"WAN_Zmq"});
    }

    const size_t dataSize =
        variable.PayloadSize() +
        m_BP3Serializer->GetBPIndexSizeInData(variable.m_Name, blockInfo.Count);
    format::BP3Base::ResizeResult resizeResult = m_BP3Serializer->ResizeBuffer(
        dataSize, "in call to variable " + variable.m_Name + " PutSync");

    if (resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        // Close buffer here?
        m_BP3Serializer->CloseStream(m_IO);
        auto &buffer = m_BP3Serializer->m_Data.m_Buffer;

        m_DataMan->WriteWAN(buffer, 0);

        // set relative position to clear buffer
        m_BP3Serializer->ResetBuffer(m_BP3Serializer->m_Data);
        // new group index
        m_BP3Serializer->PutProcessGroupIndex(m_IO.m_Name, m_IO.m_HostLanguage,
                                              {"WAN_zmq"});
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP3Serializer->PutVariableMetadata(variable, blockInfo);
    m_BP3Serializer->PutVariablePayload(variable, blockInfo);
}

template <class T>
void DataManWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    PutSyncCommon(variable, values);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
