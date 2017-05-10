/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.tcc template implementations with fix types and specializations
 *
 *  Created on: May 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_IO_TCC_
#define ADIOS2_CORE_IO_TCC_

#include "IO.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iostream>
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"

namespace adios
{

template <class T>
Variable<T> &IO::DefineVariable(const std::string &name, const Dims shape,
                                const Dims start, const Dims count,
                                const bool constantShape)
{
    if (m_DebugMode == true)
    {
        if (VariableExists(name) == true)
        {
            std::invalid_argument("ERROR: variable " + name +
                                  " exists in IO object " + m_Name +
                                  ", in call to DefineVariable\n");
        }
    }

    auto &variableMap = GetVariableMap<T>();
    const unsigned int size = variableMap.size();
    variableMap.emplace(size, Variable<T>(name, shape, start, count,
                                          constantShape, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(GetType<T>(), size));
    return variableMap.at(size);
}

template <class T>
Variable<T> &IO::GetVariable(const std::string &name)
{
    return GetVariableMap<T>().at(GetVariableIndex(name));
}

// PRIVATE
template <>
std::map<unsigned int, Variable<char>> &IO::GetVariableMap()
{
    return m_Char;
}

template <>
std::map<unsigned int, Variable<unsigned char>> &IO::GetVariableMap()
{
    return m_UChar;
}

template <>
std::map<unsigned int, Variable<short>> &IO::GetVariableMap()
{
    return m_Short;
}

template <>
std::map<unsigned int, Variable<unsigned short>> &IO::GetVariableMap()
{
    return m_UShort;
}

template <>
std::map<unsigned int, Variable<int>> &IO::GetVariableMap()
{
    return m_Int;
}

template <>
std::map<unsigned int, Variable<unsigned int>> &IO::GetVariableMap()
{
    return m_UInt;
}

template <>
std::map<unsigned int, Variable<long int>> &IO::GetVariableMap()
{
    return m_LInt;
}

template <>
std::map<unsigned int, Variable<unsigned long int>> &IO::GetVariableMap()
{
    return m_ULInt;
}

template <>
std::map<unsigned int, Variable<long long int>> &IO::GetVariableMap()
{
    return m_LLInt;
}

template <>
std::map<unsigned int, Variable<unsigned long long int>> &IO::GetVariableMap()
{
    return m_ULLInt;
}

template <>
std::map<unsigned int, Variable<float>> &IO::GetVariableMap()
{
    return m_Float;
}

template <>
std::map<unsigned int, Variable<double>> &IO::GetVariableMap()
{
    return m_Double;
}

template <>
std::map<unsigned int, Variable<long double>> &IO::GetVariableMap()
{
    return m_LDouble;
}

template <>
std::map<unsigned int, Variable<cfloat>> &IO::GetVariableMap()
{
    return m_CFloat;
}

template <>
std::map<unsigned int, Variable<cdouble>> &IO::GetVariableMap()
{
    return m_CDouble;
}

template <>
std::map<unsigned int, Variable<cldouble>> &IO::GetVariableMap()
{
    return m_CLDouble;
}

} // end namespace adios

#endif /* ADIOS2_CORE_IO_TCC_ */
