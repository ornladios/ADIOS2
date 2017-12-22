/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIWriter.tcc implementation of template functions with known type
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */
#ifndef ADIOS2_ENGINE_INSITUMPIWRITER_TCC_
#define ADIOS2_ENGINE_INSITUMPIWRITER_TCC_

#include "InSituMPIWriter.h"

#include <iostream>

namespace adios2
{

template <class T>
void InSituMPIWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    // set variable
    variable.SetData(values);
    throw std::runtime_error(
        "ERROR: InSituMPI engine does not allow for PutSync().");
}

template <class T>
void InSituMPIWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " PutDeferred("
                  << variable.m_Name << ")\n";
    }

    const size_t dataSize = m_BP3Serializer.GetVariableBPIndexSize(
        variable.m_Name, variable.m_Count);
    format::BP3Base::ResizeResult resizeResult = m_BP3Serializer.ResizeBuffer(
        dataSize, "in call to variable " + variable.m_Name + " PutSync");

    if (resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        throw std::runtime_error("ERROR: InSituMPI write engine PutDeferred(" +
                                 variable.m_Name +
                                 ") caused Flush which is not handled).");
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)
    // we only need the metadata structure but this is the granularity of the
    // function call
    m_BP3Serializer.PutVariableMetadata(variable);

    // Remember this variable to make the send request in PerformPuts()
    m_BP3Serializer.m_DeferredVariables.push_back(variable.m_Name);
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIWRITER_TCC_ */
