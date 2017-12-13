/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.tcc
 *
 *  Created on: Dec 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_

#include "DataManReader.h"

namespace adios2
{

template <class T>
void DataManReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    if (m_UseFormat == "BP")
    {
        variable.SetData(data);
        m_BP3Deserializer.GetSyncVariableDataFromStream(
            variable, m_BP3Deserializer.m_Data);
    }
}

// TODO: let;s try with GetSync first, GetDeferred, PerformGets is just a
// wrapper
template <class T>
void DataManReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    m_BP3Deserializer.GetDeferredVariable(variable, data);
    m_BP3Deserializer.m_PerformedGets = false;
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_ */
