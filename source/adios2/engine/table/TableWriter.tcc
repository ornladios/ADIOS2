/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.tcc implementation of template functions with known type
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#ifndef ADIOS2_ENGINE_TABLEWRITER_TCC_
#define ADIOS2_ENGINE_TABLEWRITER_TCC_

#include "TableWriter.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <>
void TableWriter::PutSyncCommon<std::string>(Variable<std::string> &variable,
                                             const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (not var)
    {
        var = m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                  {LocalValueDim});
    }
    m_SubEngine->Put(var, data, Mode::Sync);
}

template <>
void TableWriter::PutDeferredCommon<std::string>(
    Variable<std::string> &variable, const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (not var)
    {
        var = m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                  {LocalValueDim});
    }
    m_SubEngine->Put(var, data, Mode::Deferred);
}

template <class T>
void TableWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutSyncCommon " << m_MpiRank << " begin"
                  << std::endl;
    }
    PutDeferredCommon(variable, data);
    PerformPuts();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutSyncCommon " << m_MpiRank << " end"
                  << std::endl;
    }
}

template <class T>
void TableWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutDeferredCommon " << m_MpiRank << " begin"
                  << std::endl;
    }

    if (variable.m_SingleValue)
    {
        variable.m_Shape = Dims(1, 1);
        variable.m_Start = Dims(1, 0);
        variable.m_Count = Dims(1, 1);
    }
    variable.SetData(data);

    auto aggregatorIndices =
        WhatAggregatorIndices(variable.m_Start, variable.m_Count);

    for (auto i : aggregatorIndices)
    {
        auto serializer = m_Serializers[i];
        serializer->PutVar(variable, m_Name, CurrentStep(), m_MpiRank, "",
                           Params());
        if (serializer->LocalBufferSize() > m_SerializerBufferSize / 2)
        {
            if (m_MpiSize > 1)
            {
                auto localPack = serializer->GetLocalPack();
                auto reply = m_SendStagingMan.Request(
                    localPack->data(), localPack->size(),
                    serializer->GetDestination());
                serializer->NewWriterBuffer(m_SerializerBufferSize);
                if (m_Verbosity >= 1)
                {
                    std::cout << "TableWriter::PutDeferredCommon Rank "
                              << m_MpiRank << " Sent a package of size "
                              << localPack->size() << " to "
                              << serializer->GetDestination()
                              << " and received reply " << reply->data()[0]
                              << std::endl;
                }
            }
            else
            {
                auto localPack = serializer->GetLocalPack();
                m_Deserializer.PutPack(localPack);
                serializer->NewWriterBuffer(m_SerializerBufferSize);
                PutAggregatorBuffer();
                PutSubEngine();
            }
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutDeferredCommon " << m_MpiRank << " end"
                  << std::endl;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TABLEWRITER_TCC_ */
