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

template <class T>
void TableWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <class T>
void TableWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{

    if (variable.m_SingleValue)
    {
        variable.m_Shape = Dims(1, 1);
        variable.m_Start = Dims(1, 0);
        variable.m_Count = Dims(1, 1);
    }
    variable.SetData(data);

    size_t total_size =
        std::accumulate(variable.m_Count.begin(), variable.m_Count.end(),
                        sizeof(T), std::multiplies<size_t>());
    m_DataManSerializer.New(total_size + 1024);
    m_DataManSerializer.PutVar(variable, m_Name, CurrentStep(), m_MpiRank,
                               m_AllAddresses[m_MpiRank], Params());
    auto localPack = m_DataManSerializer.GetLocalPack();

    auto ranks = WhatRanks(variable.m_Start, variable.m_Count);

    for (const auto r : ranks)
    {
        m_SendStagingMan.Request(*localPack, m_AllAddresses[r]);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TABLEWRITER_TCC_ */
