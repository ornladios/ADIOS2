/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4FileReader.tcc
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP4_BP4FILEREADER_TCC_
#define ADIOS2_ENGINE_BP4_BP4FILEREADER_TCC_

#include "BP4FileReader.h"

namespace adios2
{
namespace core
{
namespace engine
{
template <>
inline void BP4FileReader::GetSyncCommon(Variable<std::string> &variable,
                                        std::string *data)
{
    variable.SetData(data);
    m_BP4Deserializer.GetValueFromMetadata(variable);
}

template <class T>
inline void BP4FileReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.SetData(data);

    if (variable.m_SingleValue)
    {
        m_BP4Deserializer.GetValueFromMetadata(variable);
        return;
    }

    const std::map<std::string, helper::SubFileInfoMap> variableSubfileInfo =
        m_BP4Deserializer.GetSyncVariableSubFileInfo(variable);

    ReadVariables(variableSubfileInfo);
}

template <class T>
void BP4FileReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    m_BP4Deserializer.GetDeferredVariable(variable, data);
    m_BP4Deserializer.m_PerformedGets = false;
}

} // end namespace engine
}// end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4FILEREADER_TCC_ */
