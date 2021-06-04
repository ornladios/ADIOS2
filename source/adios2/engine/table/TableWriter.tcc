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
    if (!var)
    {
        var = &m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                   {LocalValueDim});
    }
    m_SubEngine->Put(*var, data, Mode::Sync);
}

template <>
void TableWriter::PutDeferredCommon<std::string>(
    Variable<std::string> &variable, const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                   {LocalValueDim});
    }
    m_SubEngine->Put(*var, data, Mode::Deferred);
}

template <class T>
void TableWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <class T>
void TableWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    auto var = m_SubIO.InquireVariable<T>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<T>(variable.m_Name, variable.m_Shape);
        for (const auto &i : m_IO.m_TransportsParameters)
        {
            auto itVar = i.find("Variable");
            if (itVar != i.end() && itVar->second == variable.m_Name)
            {
                auto itAccuracy = i.find("Accuracy");
                if (itAccuracy != i.end())
                {
                    var->AddOperation(m_Operator,
                                      {{adios2::ops::zfp::key::accuracy,
                                        itAccuracy->second}});
                }
                auto itIndexing = i.find("Index");
                if (itIndexing != i.end())
                {
                    m_Indexing[variable.m_Name] = true;
                }
                else
                {
                    m_Indexing[variable.m_Name] = false;
                }
            }
        }
    }
    if (m_Indexing[variable.m_Name])
    {
        // TODO: implement indexing
    }
    else
    {
        var->SetSelection({variable.m_Start, variable.m_Count});
        m_SubEngine->Put(*var, data, Mode::Deferred);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TABLEWRITER_TCC_ */
