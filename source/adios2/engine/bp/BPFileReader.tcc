/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.tcc
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP_BPFILEREADER_TCC_
#define ADIOS2_ENGINE_BP_BPFILEREADER_TCC_

#include "BPFileReader.h"

namespace adios2
{

template <class T>
Variable<T> *
BPFileReader::InquireVariableCommon(const std::string &variableName)
{
    const DataMap &variablesDataMap = m_IO.GetVariablesDataMap();
    if (variablesDataMap.count(variableName) == 0)
    {
        return nullptr;
    }

    return &m_IO.GetVariable<T>(variableName);
}

template <class T>
void BPFileReader::ReadCommon(Variable<T> &variable, T *values)
{
    // figure out subfiles and start and end points
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP_BPFILEREADER_TCC_ */
