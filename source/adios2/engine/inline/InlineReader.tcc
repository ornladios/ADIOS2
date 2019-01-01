/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InlineReader.tcc
 *
 *  Created on: Nov 16, 2018
 *      Author: Aron Helser aron.helser@kitware.com
 */

#ifndef ADIOS2_ENGINE_INLINEREADER_TCC_
#define ADIOS2_ENGINE_INLINEREADER_TCC_

#include "InlineReader.h"
#include "InlineWriter.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void InlineReader::GetSyncCommon(Variable<std::string> &variable,
                                        std::string *data)
{
    variable.m_Data = data;
    auto blockInfo = variable.m_BlocksInfo.back();
    if (blockInfo.IsValue)
    {
        *data = blockInfo.Value;
    }
    else
    {
        *data = blockInfo.Data[0];
    }
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << "     GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
inline void InlineReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.m_Data = data;
    auto blockInfo = variable.m_BlocksInfo.back();
    if (blockInfo.IsValue)
    {
        *data = blockInfo.Value;
    }
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << "     GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
void InlineReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << "     GetDeferred("
                  << variable.m_Name << ")\n";
    }
    m_NeedPerformGets = true;
}

template <class T>
inline typename Variable<T>::Info *
InlineReader::GetBlockSyncCommon(Variable<T> &variable)
{
    InlineWriter &writer =
        dynamic_cast<InlineWriter &>(m_IO.GetEngine(m_WriterID));
    writer.AddReadVariable(variable.m_Name);
    if (m_DebugMode)
    {
        if (variable.m_BlockID >= variable.m_BlocksInfo.size())
        {
            throw std::invalid_argument(
                "ERROR: selected BlockID " +
                std::to_string(variable.m_BlockID) +
                " is above range of available blocks in GetBlockSync\n");
        }
    }
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << "     GetBlockSync("
                  << variable.m_Name << ")\n";
    }
    return &variable.m_BlocksInfo[variable.m_BlockID];
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_INLINEREADER_TCC_
